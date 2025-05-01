#include "globals.h"
#include <signal.h>
#include <pthread.h>

#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include "nlohmann/json.hpp"

#include "io.h"
#include "threaded_queue.h"
#include "msg.h"
#include "volume_manager.h"
#include "utils.h"
#include "controller.h"
#include "config.h"

using namespace std;
using json=nlohmann::json;

shared_ptr<IO> io;
shared_ptr<ThreadedQueue<Msg>> msgQueue;
shared_ptr<VolumeManager> mgr;
shared_ptr<Controller> controller;
bool running;
pthread_t thSignal;

const string CONFIG_PATH = "config.json";
Config cfg;

json storeConfigToJSON(Config &cfg) {
    json doc;
    doc["port"] = cfg.port;
    doc["baud"] = cfg.baud;
    switch (cfg.parity)
    {
    case Config::Parity::EVEN:
        doc["parity"] = "EVEN";
        break;
    case Config::Parity::ODD:
        doc["parity"] = "ODD";
        break;
    default:
        doc["parity"] = "NONE";
        break;
    }
    doc["channels"] = json::array();
    for (int i = 0; i < cfg.channels.size(); i++) {
        json ch;
        ch["id"] = i;
        ch["sessions"] = json::array();
        for (string &session : cfg.channels[i]) {
            ch["sessions"].push_back(session);
        }
        doc["channels"].push_back(ch);
    }
    return doc;
}

void writeConfig(Config &cfg) {
    LOG("writing config: " << CONFIG_PATH);
    json data = storeConfigToJSON(cfg);
    ofstream out(CONFIG_PATH);
    out << data.dump(2);
    out.close();
}

void parseConfigFromJSON(json &doc, Config &cfg) {
    if (doc.contains("port")) {
        cfg.port = doc["port"];
    }
    if (doc.contains("baud")) {
        cfg.baud = doc["baud"];
    }
    if (doc.contains("parity")) {
        string p = doc["parity"];
        if (p == "NONE") cfg.parity = Config::Parity::NONE;
        else if (p == "EVEN") cfg.parity = Config::Parity::EVEN;
        else if (p == "ODD") cfg.parity = Config::Parity::ODD;
    }
    if (doc.contains("channels")) {
        json &channels = doc["channels"];
        cfg.channels.clear();
        cfg.channels.resize(NUM_CHANNELS);
        for (json &channel : channels) {
            int id = channel["id"];
            if (id < 0 || id >= NUM_CHANNELS) continue;
            json sessions = channel["sessions"];
            for (string session : sessions) {
                cfg.channels[id].push_back(session);
            }
        }
    }
}

void readConfig(Config &cfg) {
    LOG("reading config: " << CONFIG_PATH);
    ifstream in(CONFIG_PATH);
    json raw;
    try{
        raw = json::parse(in);
    } catch(json::parse_error& e) {
        ERR(e.what());
        return;
    }
    cfg.channels.resize(NUM_CHANNELS);
    parseConfigFromJSON(raw, cfg);
}

void* signalThread(void *param) {
    int sig;
    sigset_t *set = static_cast<sigset_t*>(param);

    while (1) {
        sigwait(set, &sig);
        if (sig == SIGINT || sig == SIGTERM) {
            LOG(sig << " received in signal thread");
            running = false;
            io->stop();
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

int main() {
    LOG("Arduino Volume Controller");
    running = true;

    // block SIGINT in current thread, new threads should inherit the changes
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&thSignal, NULL, signalThread, &set);

    readConfig(cfg);

    msgQueue = make_shared<ThreadedQueue<Msg>>();
    io = make_shared<IO>(msgQueue, cfg);
    mgr = make_shared<VolumeManager>(msgQueue);

    mgr->init();

    if(!io->init()) {
        LOG("failed to initialize IO");
        return 0;
    }
    io->reopenSerialPort();

    controller = make_shared<Controller>(io, mgr, cfg);

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
            goto end_while;

        case MsgType::SERIAL_CONNECTED:
            break;
        case MsgType::SERIAL_DISCONNECTED:
            break;
        case MsgType::SERIAL_DATA:
            controller->parseData(msg.data);
            break;

        case MsgType::PIPE_CONNECTED:
            break;
        case MsgType::PIPE_DISCONNECTED:
            break;
        case MsgType::PIPE_DATA:
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
        
        default:
            break;
        }
        
    }
    end_while: ;

    io->wait();
    mgr->wait();
    pthread_join(thSignal, NULL);

    return 0;
}