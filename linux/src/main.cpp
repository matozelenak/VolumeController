#include "globals.h"
#include <signal.h>

#include <iostream>
#include <string>
#include <memory>

#include "io.h"
#include "threaded_queue.h"
#include "msg.h"

using namespace std;

shared_ptr<IO> io;
shared_ptr<ThreadedQueue<Msg>> msgQueue;
bool running;

void signal_handler(int sig) {
    switch(sig) {
        case SIGINT:
            io->stop();
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
            LOG("data: " << msg.data);
            switch (msg.type)
            {
            case MsgType::EXIT:
                goto end_while;
            case MsgType::SERIAL_DATA:
                break;
            case MsgType::PIPE_DATA:
                break;
            default:
                break;
            }
        }
        end_while: ;

        msgQueue->unlock();
    }

    io->wait();

    return 0;
}