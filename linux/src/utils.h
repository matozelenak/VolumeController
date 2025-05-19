#pragma once
#include "globals.h"
#include "config.h"
#include "nlohmann/json.hpp"

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

    static nlohmann::json storeConfigToJSON(Config &cfg);
    static void writeConfig(Config &cfg);
    static void parseConfigFromJSON(nlohmann::json &doc, Config &cfg);
    static void readConfig(Config &cfg);

private:
    static char _getCmdTypeChar(CMDTYPE type);
    static bool _parseNumber(std::string &data, int &i, int start, size_t count);
    static bool _parseHeader(std::string &data, char &cmd, int &colonIndex);
};