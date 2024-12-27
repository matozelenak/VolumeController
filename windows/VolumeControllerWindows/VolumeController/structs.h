#pragma once
#include "globals.h"

#include <string>
#include <vector>
#include <Windows.h>
#include "json/json.hpp"

struct Config {
	boolean error = false;
	std::wstring portName = L"";
	DWORD baudRate = 115200;
	BYTE parity = EVENPARITY;
	std::vector<std::vector<std::wstring>> channels;
	//nlohmann::json doc;
};