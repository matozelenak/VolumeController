#pragma once

#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
    #define LOG(x) std::cout << x << std::endl;
    #define ERR(x) std::cerr << "[ERROR] " << x << ": " << strerror(errno) << std::endl;
#else
    #define LOG(x) 
    #define ERR(x) 
#endif

#define PIPE_PATH "/tmp/VolumeControllerPipe"

class IO;
template <typename T> class ThreadedQueue;