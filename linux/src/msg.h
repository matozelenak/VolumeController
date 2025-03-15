#pragma once

#include <string>

enum class MsgType {
    EXIT, SERIAL_DATA, PIPE_DATA
};

struct Msg {
    MsgType type;
    std::string data;
};