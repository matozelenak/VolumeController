#pragma once
#include "globals.h"

#include <string>

enum class MsgType {
    EXIT, SERIAL_DATA, SERIAL_CONNECTED, SERIAL_DISCONNECTED,
        PIPE_DATA, PIPE_CONNECTED, PIPE_DISCONNECTED,
        PA_CONTEXT_READY, PA_CONTEXT_DISCONNECTED
};

struct Msg {
    MsgType type;
    std::string data;
};