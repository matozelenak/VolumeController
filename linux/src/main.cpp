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

        while (!msgQueue->empty()) {
            Msg msg = msgQueue->front();
            msgQueue->pop();
            LOG("[MSG] " << (int) msg.type << " data: " << msg.data);
            switch (msg.type)
            {
            case MsgType::EXIT:
                msgQueue->unlock();
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
        
        msgQueue->unlock();
    }
    end_while: ;

    io->wait();
    mgr->wait();
    pthread_join(thSignal, NULL);

    return 0;
}