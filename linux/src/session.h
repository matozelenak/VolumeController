#pragma once
#include "globals.h"

#include <string>

struct Session {
    int pid;
    int index;
    std::string name;
    std::string description;
    double volume;
    bool muted;
};