#pragma once
#include "globals.h"

#include <string>

enum class MsgType {
    EXIT,
    SERIAL_DATA, SERIAL_CONNECTED, SERIAL_DISCONNECTED,
    PIPE_DATA, PIPE_CONNECTED, PIPE_DISCONNECTED,
    PA_CONTEXT_READY, PA_CONTEXT_DISCONNECTED,
    SINK_ADDED, SINK_REMOVED, SINK_CHANGED,
    SINK_INPUT_ADDED, SINK_INPUT_REMOVED, SINK_INPUT_CHANGED,
    LIST_SINKS_COMPLETE, LIST_SINK_INPUTS_COMPLETE,
    DEFAULT_SINK_CHANGED, DEFAULT_SOURCE_CHANGED,
    CLICK_OPENGUI, CLICK_RESCAN, CLICK_RECONNECT, CLICK_EXIT,
    CONFIG_CHANGED
};

struct Msg {
    MsgType type;
    std::string data;
};