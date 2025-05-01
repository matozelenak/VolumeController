#pragma once
#include "globals.h"

#include <string>
#include <vector>

struct Config {
    enum class Parity {NONE, EVEN, ODD};

    std::string port = "";
    int baud = 115200;
    Parity parity = Parity::NONE;
    std::vector<std::vector<std::string>> channels;
};