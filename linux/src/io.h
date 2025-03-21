#pragma once
#include "globals.h"
#include "threaded_queue.h"
#include "msg.h"

#include <sys/un.h>
#include <pthread.h>
#include <string>
#include <memory>

class IO {

public:
    IO(std::shared_ptr<ThreadedQueue<Msg>> msgQueue);
    ~IO();
    bool init(); // TODO config struct
    
    void stop();
    void wait();
    void reopenSerialPort();
    bool isSerialConnected();
    bool isRunning();

private:
    bool _reinitSerialPort();
    bool _closeSerialPort();
    bool _openSerialPort();
    bool _setSerialParams();

    static void* _threadRoutineStatic(void* param);
    void _threadRoutine(void* param);

    int _fdSerialPort;
    int _baudRate;
    std::string _serialPortName;
    bool _running;
    bool _isSerialConnected;
    bool _reinitSerial;
    int _fdSocket;
    sockaddr_un addr;

    pthread_t _thread;
    std::shared_ptr<ThreadedQueue<Msg>> _msgQueue;
};