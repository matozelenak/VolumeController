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

    static CMDWHAT getWhat(std::string &data);
    static CMDTYPE getType(std::string &data);
    static bool parseCmd1Value(std::string &data, char &cmd, int &ch, int &val);
    static bool parseCmdAllValues(std::string &data, char &cmd, int vals[]);
    static std::string makeRequest(CMDTYPE type);
    static std::string makeCmd1Value(CMDTYPE type, int ch, int val);
    static std::string makeCmdAllValues(CMDTYPE type, int vals[]);

private:
    static char _getCmdTypeChar(CMDTYPE type);
    static bool _parseNumber(std::string &data, int &i, int start, size_t count);
    static bool _parseHeader(std::string &data, char &cmd, int &colonIndex);
};