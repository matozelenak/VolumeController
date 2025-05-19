#pragma once

#define DEBUG_OUTPUT
#define INFO_OUTPUT

#ifdef INFO_OUTPUT
    #define LOG(x) std::cout << x << std::endl;
    #define ERR(x) std::cerr << "[ERROR] " << x << ": " << strerror(errno) << std::endl;
#else
    #define LOG(x) 
    #define ERR(x) 
#endif

#ifdef DEBUG_OUTPUT
    #define DBG(x) std::cout << x << std::endl; 
#else
    #define DBG(x) 
#endif

#define PIPE_PATH "/tmp/VolumeControllerPipe"
#define APPLICATION_NAME "VolumeController"
#define NUM_CHANNELS 6

enum class CMDWHAT;
enum class CMDTYPE;

class IO;
template <typename T> class ThreadedQueue;
class VolumeManager;
class Utils;
class Controller;
class Channel;