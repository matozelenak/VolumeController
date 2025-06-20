#pragma once
#include "globals.h"
#include "threaded_queue.h"
#include "msg.h"
#include "config.h"

#include <sys/un.h>
#include <pthread.h>
#include <string>
#include <memory>
#include <libudev.h>

class IO {

public:
    IO(std::shared_ptr<ThreadedQueue<Msg>> msgQueue, Config &cfg);
    ~IO();
    bool init(); // TODO config struct
    
    void stop();
    void wait();
    void reopenSerialPort();
    bool isSerialConnected();
    bool isRunning();

    void sendSerial(const char *data);
    void configChanged(Config &cfg);
    void sendPipe(const char *data);

private:
    bool _reinitSerialPort();
    bool _closeSerialPort();
    bool _openSerialPort();
    bool _setSerialParams();

    void _threadRoutine(void* param);

    int _fdSerialPort;
    int _baudRate;
    Config::Parity _parity;
    std::string _serialPortName;
    bool _running;
    bool _isSerialConnected;
    bool _reinitSerial;
    int _fdSocket;
    sockaddr_un addr;
    bool _isPipeConnected;
    int _fdGUIClient;

    pthread_t _thread;
    std::shared_ptr<ThreadedQueue<Msg>> _msgQueue;

    udev *_udev;
    udev_monitor *_udev_monitor;
};