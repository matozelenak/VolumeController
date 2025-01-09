#pragma once
#include <string>
#include <codecvt>
#include <vector>

#include <Windows.h>
#include <Psapi.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "structs.h"
#include "json/json.hpp"

class Utils {

public:
	static std::wstring getProcessName(DWORD pid);
	static std::wstring getIMMDeviceProperty(CComPtr<IMMDevice> device, const PROPERTYKEY& key);
	static std::wstring printLastError(std::wstring functionName);
	static std::wstring strToWstr(std::string& str);
	static std::string wStrToStr(std::wstring& wstr);
	static std::wstring getCurrentDir();
	static std::wstring joinPathWithFileName(std::wstring path, std::wstring filename);

	static std::string makeCmd1Val(char cmd, int ch, int val);
	static std::string makeCmdAllVals(char cmd, std::vector<int>& vals);
	static int parseCmd1Val(std::string& data, int& ch);
	static std::vector<int> parseCmdAllVals(std::string& data);

	static void parseConfigFromJSON(nlohmann::json& doc, Config& config);
	static nlohmann::json storeConfigToJSON(Config& config);
	static nlohmann::json readConfig(std::string file);
	static void writeConfig(nlohmann::json& doc, std::string file);

	static void makeConsole();
	static void openGUI();
	static std::wstring makeDebugInfo(VolumeManager* vol, Controller* controller);
private:
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
};