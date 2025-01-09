#include "globals.h"
#include "utils.h"
#include "structs.h"
#include "controller.h"
#include "volume_manager.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <locale>
#include <sstream>
#include <fstream>

#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "json/json.hpp"

using namespace std;

wstring Utils::getProcessName(DWORD pid) {
	wstring filename = L"";
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess) {
		WCHAR processName[MAX_PATH];
		if (GetModuleFileNameExW(hProcess, NULL, processName, MAX_PATH)) {
			filename = PathFindFileNameW(processName);
		}
		else {
			cerr << " (GetModuleFileName) error: " << GetLastError() << endl;
		}

		CloseHandle(hProcess);
	}
	else {
		cerr << "error (OpenProcess): " << GetLastError() << ", pid: " << pid << endl;
	}

	return filename;
}

wstring Utils::getIMMDeviceProperty(CComPtr<IMMDevice> device, const PROPERTYKEY& key) {
	CComPtr<IPropertyStore> props;
	device->OpenPropertyStore(STGM_READ, &props);

	PROPVARIANT prop;
	PropVariantInit(&prop);
	props->GetValue(key, &prop);

	if (prop.vt != VT_EMPTY) {
		wstring ret = prop.pwszVal;
		PropVariantClear(&prop);
		props.Release();
		return ret;
	}

	PropVariantClear(&prop);
	props.Release();
	return wstring();
}

wstring Utils::printLastError(wstring functionName) {
	wstring result = L"[Error] ";
	result += functionName;
	result += L": ";
	LPVOID buff;
	DWORD size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, (LPWSTR)&buff, 0, NULL);
	if (size > 0) {
		result += (LPCWSTR) buff;
	}
	else {
		result += L"unknown error";
	}
	if (buff) {
		LocalFree(buff);
	}
	wcerr << result << endl;
	return result;
}


wstring_convert<codecvt_utf8<wchar_t>> Utils::utf8_conv;

wstring Utils::strToWstr(string& str) {
	return utf8_conv.from_bytes(str);
}

string Utils::wStrToStr(wstring& wstr) {
	return utf8_conv.to_bytes(wstr);
}

wstring Utils::getCurrentDir() {
	WCHAR buffer[MAX_PATH];
	DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
	if (length == 0) {
		Utils::printLastError(L"GetModuleFileNameW");
		return L"";
	}
	if (FAILED(PathRemoveFileSpecW(buffer))) {
		return L"";
	}
	return buffer;
}

wstring Utils::joinPathWithFileName(wstring path, wstring filename) {
	path += L"\\";
	path += filename;
	return path;
}

string Utils::makeCmd1Val(char cmd, int ch, int val) {
	stringstream ss;
	ss << "!" << cmd << ch << ":" << ch << "\n";
	return ss.str();
}

string Utils::makeCmdAllVals(char cmd, vector<int>& vals) {
	stringstream ss;
	ss << "!" << cmd << ":";
	for (int i : vals)
		ss << i << "|";
	ss << "\n";
	return ss.str();
}

int Utils::parseCmd1Val(string& data, int& chID) {
	size_t colon = data.find(':', 0);
	string strChID = data.substr(2, colon);
	chID = stoi(strChID);
	string strValue = data.substr(colon + 1, string::npos);
	int value = stoi(strValue);
	return value;
}

vector<int> Utils::parseCmdAllVals(string& data) {
	vector<int> result;
	size_t begin = data.find(':') + 1;
	int pos = 0, prev = -1 + begin;
	while (1) {
		pos = data.find('|', prev + 1);
		if (pos == string::npos) break;
		string strVal = data.substr(prev + 1, pos);
		result.push_back(stoi(strVal));
		prev = pos;
	}
	return result;
}

void Utils::parseConfigFromJSON(nlohmann::json& doc, Config& config) {
	DBG_PRINT("Parsing config..." << endl);
	if (doc.contains("port")) {
		string s = doc["port"];
		config.portName = Utils::strToWstr(s);
	}

	if (doc.contains("baud"))
		config.baudRate = doc["baud"];

	if (doc.contains("parity")) {
		string parity = doc["parity"];
		if (parity == "EVEN")
			config.parity = EVENPARITY;
		else if (parity == "ODD")
			config.parity = ODDPARITY;
		else
			config.parity = NOPARITY;
	}

	if (doc.contains("channels")) {
		config.channels.clear();
		for (int i = 0; i < 6; i++)
			config.channels.push_back(vector<wstring>());

		nlohmann::json& cfgChannels = doc["channels"];
		for (auto ch : cfgChannels) {
			int id = ch["id"];
			if (id >= 0 && id <= 5) {
				for (string strSession : ch["sessions"]) {
					config.channels[id].push_back(Utils::strToWstr(strSession));
				}

			}
		}
	}
}

nlohmann::json Utils::storeConfigToJSON(Config& config) {
	nlohmann::json doc;
	doc["port"] = Utils::wStrToStr(config.portName);
	doc["baud"] = (int)config.baudRate;
	if (config.parity == EVENPARITY)
		doc["parity"] = "EVEN";
	else if (config.parity == ODDPARITY)
		doc["parity"] = "ODD";
	else
		doc["parity"] = "NONE";

	nlohmann::json channels = nlohmann::json::array();
	for (int i = 0; i < config.channels.size(); i++) {
		vector<wstring>& vecChannel = config.channels[i];
		nlohmann::json channel;
		channel["id"] = i;
		channel["sessions"] = nlohmann::json::array();
		for (wstring& wstrSessionName : vecChannel)
			channel["sessions"].push_back(Utils::wStrToStr(wstrSessionName));

		channels.push_back(channel);
	}
	doc["channels"] = channels;
	return doc;
}

nlohmann::json Utils::readConfig(string file) {
	DBG_PRINT("Reading config file..." << endl);
	ifstream f(Utils::joinPathWithFileName(Utils::getCurrentDir(), Utils::strToWstr(file)));
	nlohmann::json doc;
	try {
		doc = nlohmann::json::parse(f);
	}
	catch (const nlohmann::json::parse_error& e) {
		DBG_PRINT("JSON parse error: " << e.what() << endl);
	}
	return doc;
}

void Utils::writeConfig(nlohmann::json& doc, string file) {
	DBG_PRINT("Saving config to file..." << endl);
	ofstream out(Utils::joinPathWithFileName(Utils::getCurrentDir(), Utils::strToWstr(file)));
	out << doc.dump(2) << endl;
	out.close();
}

void Utils::makeConsole() {
	AllocConsole();

	// std::cout, std::clog, std::cerr, std::cin
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
}

void Utils::openGUI() {
	ShellExecuteW(NULL, NULL, Utils::joinPathWithFileName(Utils::getCurrentDir(),
		GUI_EXE_PATH).c_str(), NULL, NULL, SW_SHOW);
}

wstring Utils::makeDebugInfo(VolumeManager* vol, Controller* controller) {
	wstringstream messageBoxContent;
	messageBoxContent << L"==========devicePool===========" << endl;
	vector<PAudioDevice>& devicePool = vol->getDevicePool();
	for (PAudioDevice& device : devicePool) {
		messageBoxContent << L"  - " << device->getName() << endl;
	}
	messageBoxContent << L"==========sessionPool===========" << endl;
	vector<PAudioSession>& sessionPool = vol->getSessionPool();
	for (PAudioSession& session : sessionPool) {
		messageBoxContent << L"  - " << session->getName() << L" [" << session->getPID() << L"]" << endl;
	}
	messageBoxContent << L"==========channels============" << endl;
	vector<Channel>& channels = controller->getChannels();
	int i = 1;
	for (Channel& channel : channels) {
		messageBoxContent << L" " << i << L"." << endl;
		i++;
		vector<PISession>& sessions = channel.getSessions();
		for (PISession& session : sessions) {
			messageBoxContent << L"  - " << session->getName() << endl;
		}
	}
	messageBoxContent << "==========================" << endl;
	return messageBoxContent.str();
}