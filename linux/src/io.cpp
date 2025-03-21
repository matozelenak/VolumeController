#include "io.h"
#include "threaded_queue.h"
#include "msg.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>
#include <string>
#include <memory>
#include <cerrno>
#include <cstring>

IO::IO(std::shared_ptr<ThreadedQueue<Msg>> msgQueue)
    :_msgQueue(msgQueue) {
    _fdSerialPort = -1;
    _isSerialConnected = false;
    _running = false;

    _fdSocket = -1;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, PIPE_PATH, sizeof(addr.sun_path)-1);
}

IO::~IO() {
    _closeSerialPort();
}

bool IO::init() {
    // temporary
    _serialPortName = "/dev/ttyUSB0";
    _baudRate = 115200;

    _running = true;
    _reinitSerial = false;

    _fdSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_fdSocket == -1) {
        ERR("socket()");
        return false;
    }
    unlink(PIPE_PATH);
    
    if (bind(_fdSocket, (sockaddr*)&addr, sizeof(addr)) == -1) {
        ERR("bind()");
        close(_fdSocket);
        return false;
    }

    if (listen(_fdSocket, 2) == -1) {
        ERR("listen()");
        close(_fdSocket);
        return false;
    }
    LOG("socket fd: " << _fdSocket << ", bound and listenning...");

    pthread_create(&_thread, NULL, _threadRoutineStatic, this);

    return true;
}

void IO::stop() {
    _running = false;
}

void IO::wait() {
    LOG("waiting for IO thread to finish...");

    pthread_join(_thread, NULL);
}

void IO::reopenSerialPort() {
    _reinitSerial = true;
}

bool IO::isSerialConnected() {
    return _isSerialConnected;
}

bool IO::isRunning() {
    return _running;
}

bool IO::_reinitSerialPort() {
    _closeSerialPort();

    if (!_openSerialPort()) {
        LOG("failed to open serial port");
        return false;
    }
    LOG("serial port opened");

    if (!_setSerialParams()) {
        LOG("failed to set tty parameters");
        _closeSerialPort();
        return false;
    }
    LOG("tty paramaters set");

    _msgQueue->pushAndSignal(Msg{MsgType::SERIAL_CONNECTED, ""});

    return true;
}

bool IO::_closeSerialPort() {
    if (!_isSerialConnected) return false;
    LOG("closing serial port...");
    tcdrain(_fdSerialPort);
    close(_fdSerialPort);
    _isSerialConnected = false;
    _fdSerialPort = -1;
    _msgQueue->pushAndSignal(Msg{MsgType::SERIAL_DISCONNECTED, ""});
    return true;
}

bool IO::_openSerialPort() {
    if(_isSerialConnected) return false;
    LOG("opening device " << _serialPortName);
    _fdSerialPort = open(_serialPortName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (_fdSerialPort == -1) {
        ERR("serial open()");
        return false;
    }
    _isSerialConnected = true;
    return true;
}

bool IO::_setSerialParams() {
    termios tty;
    memset(&tty, 0, sizeof(termios));

    speed_t speed;
    switch (_baudRate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        default:
            LOG("unsupported baud rate: " << _baudRate);
            return false;
    }
    cfsetispeed(&tty, speed); // input baud rate
    cfsetospeed(&tty, speed); // output baud rate

    tty.c_iflag = 0; // disable input processing and software flow control
    tty.c_iflag |= IGNCR; // ignore carriage return '\r'

    tty.c_oflag = 0; // disable output processing

    tty.c_lflag = 0; // disable canonical mode, echo, signal processing

    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;   // clear data size bits
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_cflag |= PARENB;   // enable parity
    tty.c_cflag &= ~PARODD;  // set even parity
    tty.c_cflag &= ~CRTSCTS; // disable hardware flow control
    tty.c_cflag |= CREAD;    // enable receiver
    tty.c_cflag &= ~CLOCAL;  // allow modem control lines

    tty.c_cc[VMIN] = 1;  // must read at least 1 byte
    tty.c_cc[VTIME] = 0; // timeout

    if (tcsetattr(_fdSerialPort, TCSANOW, &tty) != 0) {
        ERR("tcsetattr()");
        return false;
    }

    // toggle DTR to reset arduino
    int status;
    ioctl(_fdSerialPort, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    ioctl(_fdSerialPort, TIOCMSET, &status);
    usleep(100000);
    status |= TIOCM_DTR;
    ioctl(_fdSerialPort, TIOCMSET, &status);

    return true;
}


void* IO::_threadRoutineStatic(void* param) {
    IO* instance = (IO*) param;
    instance->_threadRoutine(NULL);
    return NULL;
}

void IO::_threadRoutine(void* param) {
    LOG("IO thread started");

    // _reinitSerialPort();
    const int POLL_FDSIZE = 4;
    pollfd fds[POLL_FDSIZE];
    for (int i = 0; i < POLL_FDSIZE; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    fds[1].fd = _fdSocket;
    fds[1].events = POLLIN;
    char comBuffer[1024];
    int comBufferPosition = 0;
    while (_running) {
        if (_reinitSerial) {
            _reinitSerial = false;
            _reinitSerialPort();
        }

        if (_isSerialConnected) fds[0].fd = _fdSerialPort;
        else fds[0].fd = -1;
        // TODO if is pipe connected then set fd

        int numFDs = poll(fds, POLL_FDSIZE, 1000);
        if (numFDs == -1) {
            ERR("poll()");
        }
        else {
            // serial port
            if (fds[0].revents & POLLIN) {
                // read from serial port
                char c;
                int bytesRead = read(_fdSerialPort, &c, 1);
                if (bytesRead == -1) {
                    ERR("read()");
                }
                else if (bytesRead == 0) {
                    LOG("read() returned 0");
                }
                else {
                    if (c == '\n') {
                        comBuffer[comBufferPosition] = '\0';
                        // do smth
                        // LOG("[RX]: '" << comBuffer << "'");
                        _msgQueue->pushAndSignal(Msg{MsgType::SERIAL_DATA, std::string(comBuffer)});
                        comBufferPosition = 0;
                    }
                    else {
                        comBuffer[comBufferPosition] = c;
                        comBufferPosition++;
                    }

                    if (comBufferPosition == sizeof(comBuffer)-1) {
                        // the buffer is full, output it
                        comBuffer[comBufferPosition] = '\0';
                        // LOG("[RX]: '" << comBuffer << "'");
                        _msgQueue->pushAndSignal(Msg{MsgType::SERIAL_DATA, std::string(comBuffer)});
                        comBufferPosition = 0;
                    }
                }
            }

            // server socket
            if (fds[1].revents & POLLIN) {
                int clientFd = accept(_fdSocket, nullptr, nullptr);
                if (clientFd == -1) {
                    ERR("accept()");
                } else {
                    fds[2].fd = clientFd;
                    LOG("client accepted, fd: " << clientFd);
                    _msgQueue->pushAndSignal(Msg{MsgType::PIPE_CONNECTED, ""});
                }
            }

            // gui client socket
            if (fds[2].revents & POLLIN) {
                char buffer[1024];
                int bytesRead = read(fds[2].fd, buffer, sizeof(buffer)-1);
                if (bytesRead == -1) {
                    ERR("socket read()");
                    close(fds[2].fd);
                    fds[2].fd = -1;
                    _msgQueue->pushAndSignal(Msg{MsgType::PIPE_DISCONNECTED, ""});
                }
                else if (bytesRead == 0) {
                    LOG("socket read() returned 0");
                    close(fds[2].fd);
                    fds[2].fd = -1;
                    _msgQueue->pushAndSignal(Msg{MsgType::PIPE_DISCONNECTED, ""});
                }
                else {
                    buffer[bytesRead] = '\0';
                    _msgQueue->pushAndSignal(Msg{MsgType::PIPE_DATA, std::string(buffer)});
                }
            }
        }
    } // end while running
    LOG("IO thread stopping...");

    if (fds[2].fd != -1) {
        close(fds[2].fd);
    }
    if (_fdSocket != -1) {
        close(_fdSocket);
    }
    unlink(PIPE_PATH);
    _closeSerialPort();
    _msgQueue->pushAndSignal(Msg{MsgType::EXIT, "stop"});
}