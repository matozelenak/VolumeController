#include "globals.h"
#include <signal.h>

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

void signal_handler(int sig) {
    switch(sig) {
        case SIGINT:
            io->stop();
            // mgr->stop();
            running = false;
            break;
        default:
            break;
    }
}

int main() {
    LOG("Arduino Volume Controller");
    running = true;

    signal(SIGINT, signal_handler);

    msgQueue = make_shared<ThreadedQueue<Msg>>();
    io = make_shared<IO>(msgQueue);
    mgr = make_shared<VolumeManager>(msgQueue);

    mgr->init();

    if(!io->init()) {
        LOG("failed to initialize IO");
        return 0;
    }
    io->reopenSerialPort();

    while (running) {
        msgQueue->lock();
        while (msgQueue->empty()) {
            msgQueue->wait();
        }

        while (!msgQueue->empty()) {
            Msg msg = msgQueue->front();
            msgQueue->pop();
            LOG("[MSG] " << (int) msg.type << " data: " << msg.data);
            switch (msg.type)
            {
            case MsgType::EXIT:
                goto end_while;
            case MsgType::SERIAL_DATA:
                break;
            case MsgType::PIPE_DATA:
                break;
            case MsgType::PA_CONTEXT_READY:
                mgr->listSinks();
                break;
            case MsgType::PA_CONTEXT_DISCONNECTED:
                break;
            default:
                break;
            }
        }
        end_while: ;

        msgQueue->unlock();
    }

    io->wait();
    mgr->wait();

    return 0;
}