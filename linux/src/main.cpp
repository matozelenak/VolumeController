#include "globals.h"
#include <signal.h>
#include <pthread.h>

#include <iostream>
#include <string>
#include <memory>

#include "io.h"
#include "threaded_queue.h"
#include "msg.h"
#include "volume_manager.h"

using namespace std;

shared_ptr<IO> io;
shared_ptr<ThreadedQueue<Msg>> msgQueue;
shared_ptr<VolumeManager> mgr;
bool running;
pthread_t thSignal;

void* signalThread(void *param) {
    int sig;
    sigset_t *set = static_cast<sigset_t*>(param);

    while (1) {
        sigwait(set, &sig);
        if (sig == SIGINT) {
            LOG("SIGINT received in signal thread");
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
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&thSignal, NULL, signalThread, &set);

    msgQueue = make_shared<ThreadedQueue<Msg>>();
    io = make_shared<IO>(msgQueue);
    mgr = make_shared<VolumeManager>(msgQueue);

    mgr->init();

    if(!io->init()) {
        LOG("failed to initialize IO");
        return 0;
    }
    io->reopenSerialPort();

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
            break;

        case MsgType::PIPE_CONNECTED:
            break;
        case MsgType::PIPE_DISCONNECTED:
            break;
        case MsgType::PIPE_DATA:
            break;

        case MsgType::PA_CONTEXT_READY:
            mgr->listSinks_sync();
            mgr->listSinkInputs_sync();
            break;
        case MsgType::PA_CONTEXT_DISCONNECTED:
            break;

        case MsgType::SINK_ADDED:
            LOG("[EVENT] sink #" << msg.data << " added");
            break;
        case MsgType::SINK_CHANGED:
            LOG("[EVENT] sink #" << msg.data << " changed");
            break;
        case MsgType::SINK_REMOVED:
            LOG("[EVENT] sink #" << msg.data << " removed");
            break;

        case MsgType::SINK_INPUT_ADDED:
            LOG("[EVENT] sink input #" << msg.data << " added");
            printSinkInputInfo(stoi(msg.data));
            break;
        case MsgType::SINK_INPUT_CHANGED:
            LOG("[EVENT] sink input #" << msg.data << " changed");
            printSinkInputInfo(stoi(msg.data));
            break;
        case MsgType::SINK_INPUT_REMOVED:
            LOG("[EVENT] sink input #" << msg.data << " removed");
            break;

        case MsgType::LIST_SINKS_COMPLETE:
            LOG("[EVENT] list sinks complete");
            break;
        case MsgType::LIST_SINK_INPUTS_COMPLETE:
            LOG("[EVENT] list sink inputs complete");
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