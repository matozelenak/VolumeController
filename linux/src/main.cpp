#include "globals.h"
#include <signal.h>
#include <pthread.h>

#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>
#include "nlohmann/json.hpp"

#include "io.h"
#include "threaded_queue.h"
#include "msg.h"
#include "volume_manager.h"
#include "utils.h"
#include "controller.h"
#include "config.h"
#include "appind_lib.h"

using namespace std;
using json=nlohmann::json;

shared_ptr<IO> io;
shared_ptr<ThreadedQueue<Msg>> msgQueue;
shared_ptr<VolumeManager> mgr;
shared_ptr<Controller> controller;
bool running;
pthread_t thSignal;

const string CONFIG_NAME = "config.json";
const string APPINDLIB_NAME = "appind_lib.so";
const string ICON1_NAME = "vc_icon1.png";
const string ICON2_NAME = "vc_icon2.png";
string APPINDLIB_PATH = APPINDLIB_NAME;
string CONFIG_PATH = CONFIG_NAME;
string ICON1_PATH = ICON1_NAME;
string ICON2_PATH = ICON2_NAME;
Config cfg;

void *hAppIndLib = NULL;
libinit_t ind_libraryInit = NULL; 
seticon_t ind_setIcon = NULL; 
addmenuitem_t ind_addMenuItem = NULL; 
addmenusep_t ind_addMenuSep = NULL; 
showappind_t ind_showAppInd = NULL; 
hideappind_t ind_hideAppInd = NULL; 
destroy_t ind_destroy = NULL;
setclickcb_t ind_setClickCb = NULL; 


void* signalThread(void *param) {
    int sig;
    sigset_t *set = static_cast<sigset_t*>(param);

    while (1) {
        sigwait(set, &sig);
        if (sig == SIGINT || sig == SIGTERM) {
            LOG(sig << " received in signal thread");
            msgQueue->pushAndSignal(Msg{MsgType::EXIT, "stopped by SIGINT"});
            break;
        }
    }

    return NULL;
}

void printSinkInputInfo(int index) {
        Session &session = mgr->getSessionPool()->operator[](index);
        LOG("[SINK INPUT] " << session.name << " " << session.volume << " " << session.muted);
}

void clickCallback(string label, void *data) {
    if (label == "Open GUI") {
        msgQueue->pushAndSignal(Msg{MsgType::CLICK_OPENGUI});
    }
    else if (label == "Rescan sessions") {
        msgQueue->pushAndSignal(Msg{MsgType::CLICK_RESCAN});
    }
    else if (label == "Reconnect") {
        msgQueue->pushAndSignal(Msg{MsgType::CLICK_RECONNECT});
    }
    else if (label == "Exit") {
        msgQueue->pushAndSignal(Msg{MsgType::CLICK_EXIT});
    }
}

int main(int argc, char *argv[]) {
    LOG("Arduino Volume Controller");
    time_t t = time(0);
    cout << ctime(&t);
    LOG("running as: " << argv[0]);
    running = true;

    // create paths
    string path = argv[0];
    int slashPos = path.find_last_of('/');
    if (slashPos != string::npos) {
        path = path.replace(slashPos+1, string::npos, "");
        APPINDLIB_PATH = path + APPINDLIB_NAME;
    }
    char cwd[1024];
    memset(cwd, 0, sizeof(cwd));
    getcwd(cwd, sizeof(cwd));
    string cwdPath = cwd;
    cwdPath += "/";
    CONFIG_PATH = cwdPath + CONFIG_NAME;
    ICON1_PATH = cwdPath + ICON1_NAME;
    ICON2_PATH = cwdPath + ICON2_NAME;
    LOG("cwd: " << cwdPath);
    
    
    // block SIGINT in current thread, new threads should inherit the changes
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&thSignal, NULL, signalThread, &set);

    // load appind_lib if it exists
    hAppIndLib = dlopen(APPINDLIB_PATH.c_str(), RTLD_NOW);
    if (!hAppIndLib) {
        ERR(dlerror());
    } else {
        ind_libraryInit = (libinit_t) dlsym(hAppIndLib, LIBINIT_NAME);
        ind_setIcon = (seticon_t) dlsym(hAppIndLib, SETICON_NAME);
        ind_addMenuItem = (addmenuitem_t) dlsym(hAppIndLib, ADDMENUITEM_NAME);
        ind_addMenuSep = (addmenusep_t) dlsym(hAppIndLib, ADDMENUSEPARATOR_NAME);
        ind_showAppInd = (showappind_t) dlsym(hAppIndLib, SHOWAPPIND_NAME);
        ind_hideAppInd = (hideappind_t) dlsym(hAppIndLib, HIDEAPPIND_NAME);
        ind_destroy = (destroy_t) dlsym(hAppIndLib, DESTROY_NAME);
        ind_setClickCb = (setclickcb_t) dlsym(hAppIndLib, SETCLICKCB_NAME);
    }

    cfg.CONFIG_PATH = CONFIG_PATH;
    Utils::readConfig(cfg);

    msgQueue = make_shared<ThreadedQueue<Msg>>();
    io = make_shared<IO>(msgQueue, cfg);
    mgr = make_shared<VolumeManager>(msgQueue);

    mgr->init();

    if(!io->init()) {
        LOG("failed to initialize IO");
        return 0;
    }
    io->reopenSerialPort();

    controller = make_shared<Controller>(io, mgr, cfg, msgQueue);

    // create app indicator and its menu
    if (hAppIndLib) {
        ind_libraryInit(argc, argv, "VolumeControllerDaemon", ICON1_PATH); // TODO change icon
        ind_addMenuItem("Open GUI");
        ind_addMenuSep();
        ind_addMenuItem("Rescan sessions");
        ind_addMenuItem("Reconnect");
        ind_addMenuSep();
        ind_addMenuItem("Exit");
        ind_showAppInd();

        ind_setClickCb(&clickCallback, NULL);
    }

    while (true) {
        msgQueue->lock();
        while (msgQueue->empty()) {
            msgQueue->wait();
        }

        Msg msg = msgQueue->front();
        msgQueue->pop();
        msgQueue->unlock();
        LOG("[MSG] " << (int) msg.type << " data: '" << msg.data << "'");
        switch (msg.type)
        {
        case MsgType::EXIT:
            running = false;
            io->stop();
            goto end_while;

        case MsgType::SERIAL_CONNECTED:
            ind_setIcon(ICON2_PATH);
            break;
        case MsgType::SERIAL_DISCONNECTED:
            ind_setIcon(ICON1_PATH);
            break;
        case MsgType::SERIAL_DATA:
            controller->parseData(msg.data);
            break;

        case MsgType::PIPE_CONNECTED:
            break;
        case MsgType::PIPE_DISCONNECTED:
            break;
        case MsgType::PIPE_DATA:
            controller->handlePipeData(msg.data);
            break;

        case MsgType::PA_CONTEXT_READY:
            mgr->getDefSinkIdx();
            controller->remapChannels();
            break;
        case MsgType::PA_CONTEXT_DISCONNECTED:
            break;

        case MsgType::SINK_ADDED:
            LOG("[EVENT] sink #" << msg.data << " added");
            controller->addDevice(stoi(msg.data));
            break;
        case MsgType::SINK_CHANGED:
            LOG("[EVENT] sink #" << msg.data << " changed");
            break;
        case MsgType::SINK_REMOVED:
            LOG("[EVENT] sink #" << msg.data << " removed");
            controller->removeDevice(stoi(msg.data));
            break;

        case MsgType::SINK_INPUT_ADDED:
            LOG("[EVENT] sink input #" << msg.data << " added");
            printSinkInputInfo(stoi(msg.data));
            controller->addSession(stoi(msg.data));
            break;
        case MsgType::SINK_INPUT_CHANGED:
            LOG("[EVENT] sink input #" << msg.data << " changed");
            printSinkInputInfo(stoi(msg.data));
            break;
        case MsgType::SINK_INPUT_REMOVED:
            LOG("[EVENT] sink input #" << msg.data << " removed");
            controller->removeSession(stoi(msg.data));
            break;

        case MsgType::LIST_SINKS_COMPLETE:
            LOG("[EVENT] list sinks complete");
            break;
        case MsgType::LIST_SINK_INPUTS_COMPLETE:
            LOG("[EVENT] list sink inputs complete");
            break;

        case MsgType::DEFAULT_SINK_CHANGED:
            LOG("[EVENT] default sink changed");
            controller->defaultSinkChanged();
            break;
        case MsgType::DEFAULT_SOURCE_CHANGED:
            LOG("[EVENT] default source changed");
            break;

        case MsgType::CLICK_OPENGUI:
            system("volumecontroller-gui-linux-x64/volumecontroller-gui --no-sandbox &");
            break;
        case MsgType::CLICK_RESCAN:
            controller->remapChannels();
            break;
        case MsgType::CLICK_RECONNECT:
            io->reopenSerialPort();
            break;
        case MsgType::CLICK_EXIT:
            kill(getpid(), SIGTERM);
            break;

        case MsgType::CONFIG_CHANGED:
            io->configChanged(cfg);
            controller->configChanged(cfg);
            break;
        default:
            break;
        }
        
    }
    end_while: ;


    io->wait();
    mgr->wait();
    pthread_join(thSignal, NULL);
    if (hAppIndLib) {
        ind_destroy();
        dlclose(hAppIndLib);
    }
    LOG("\n\n")

    return 0;
}