#include "utils.h"

#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <string.h>
using namespace std;

vector<string> Utils::getSerialPorts() {
    vector<string> result;
    
    DIR *dir = opendir("/dev");
    if (dir == NULL) {
        ERR("opendir()");
        return result;
    }

    dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_CHR) continue;
        string name = entry->d_name;
        if (name.find("ttyUSB") == string::npos) continue;
        result.push_back(name);
    }

    return result;
}


CMDWHAT Utils::getWhat(string &data) {
    if (data.length() < 2) return CMDWHAT::UNKNOWN;
    if (data[0] == '?') return CMDWHAT::REQUEST;
    else if (data[0] == '!') return CMDWHAT::COMMAND;
    return CMDWHAT::UNKNOWN;
}

CMDTYPE Utils::getType(string &data) {
    if (data.length() < 2) return CMDTYPE::UNKNOWN;
    if (data[1] == 'A') return CMDTYPE::ACTIVE_CH;
    else if (data[1] == 'V') return CMDTYPE::VOLUME;
    else if (data[1] == 'M') return CMDTYPE::MUTE;
    return CMDTYPE::UNKNOWN;
}

bool Utils::parseCmd1Value(string &data, char &cmd, int &ch, int &val) {
    if (data.length() < 6) return false;
    int colonIndex;
    if (!_parseHeader(data, cmd, colonIndex)) return false;
    int _ch;
    if (!_parseNumber(data, _ch, 2, colonIndex-2)) return false;
    int _val;
    if (!_parseNumber(data, _val, colonIndex+1, string::npos)) return false;

    ch = _ch;
    val = _val;
    return true;
}

bool Utils::parseCmdAllValues(string &data, char &cmd, int vals[]) {
    if (data.length() < 15) return false;
    int colonIndex;
    if (!_parseHeader(data, cmd, colonIndex)) return false;
    int last = colonIndex;
    for (int i = 0; i < 6 /*NUM_CHANNELS */; i++) {
        int next = data.find('|', last+1);
        if (next == string::npos) return false;
        if (!_parseNumber(data, vals[i], last+1, next-last-1)) return false;
        last = next;
    }
    return true;
}

string Utils::makeRequest(CMDTYPE type) {    
    return string("?") + _getCmdTypeChar(type) + "\n";
}

string Utils::makeCmd1Value(CMDTYPE type, int ch, int val) {
    stringstream ss;
    ss << "!" << _getCmdTypeChar(type) << to_string(ch) << ":" << to_string(val) << "\n";
    return ss.str();
}

string Utils::makeCmdAllValues(CMDTYPE type, int vals[]) {
    stringstream ss;
    ss << "!" << _getCmdTypeChar(type) << ":";
    for (int i = 0; i < 6 /*NUM_CHANNELS*/; i++) {
        ss << vals[i] << "|";
    }
    ss << "\n";
    return ss.str();
}

char Utils::_getCmdTypeChar(CMDTYPE type) {
    switch (type)
    {
    case CMDTYPE::ACTIVE_CH:
        return 'A';
    case CMDTYPE::VOLUME:
        return 'V';
    case CMDTYPE::MUTE:
        return 'M';
    default:
        return 'X';
    }
}

bool Utils::_parseNumber(string &data, int &i, int start, size_t count) {
    string str = data.substr(start, count);
    try {
        i = stoi(str);
    } catch (std::invalid_argument const& ex) {
        cout << "stoi() invalid argument" << endl;;
        return false;
    } catch (std::out_of_range const& ex) {
        cout << "stoi() out of range" << endl;
        return false;
    }
    return true;
}

bool Utils::_parseHeader(string &data, char &cmd, int &colonIndex) {
    char _cmd = data[1];
    int _colonIndex = data.find(':');
    if (_colonIndex == string::npos) return false;
    cmd = _cmd;
    colonIndex = _colonIndex;
    return true;
}