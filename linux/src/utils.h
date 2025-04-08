#pragma once
#include "globals.h"

#include <vector>
#include <string>

enum class CMDWHAT {
    UNKNOWN, REQUEST, COMMAND
};

enum class CMDTYPE {
    UNKNOWN, ACTIVE_CH, VOLUME, MUTE
};

class Utils {
public:
    static std::vector<std::string> getSerialPorts();

    CMDWHAT getWhat(std::string &data);
    CMDTYPE getType(std::string &data);
    bool parseCmd1Value(std::string &data, char &cmd, int &ch, int &val);
    bool parseCmdAllValues(std::string &data, char &cmd, int vals[]);
    std::string makeRequest(CMDTYPE type);
    std::string makeCmd1Value(CMDTYPE type, int ch, int val);
    std::string makeCmdAllValues(CMDTYPE type, int vals[]);

private:
    char _getCmdTypeChar(CMDTYPE type);
    bool _parseNumber(std::string &data, int &i, int start, size_t count);
    bool _parseHeader(std::string &data, char &cmd, int &colonIndex);
};