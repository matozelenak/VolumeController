#include "volume_manager.h"
#include "io.h"
#include "controller.h"
#include "utils.h"
#include "structs.h"
#include "session_notification.h"
#include "globals.h"

#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <shellapi.h>
#include <fstream>
#include <codecvt>
#include "json/json.hpp"

using json = nlohmann::json;
using namespace std;

const char* CONFIG_FILE = "config.json";
void makeConsole();

VolumeManager* vol = nullptr;
IO* io = nullptr;
Controller* controller = nullptr;
Config config;

HWND hWnd = NULL;
HMENU hTrayMenu = NULL;

void readConfig() {
	config.doc = json();
	config.error = false;
	config.channels.clear();
	for (int i = 0; i < 6; i++)
		config.channels.push_back(vector<wstring>());

	ifstream f(CONFIG_FILE);
	json doc;
	try {
		doc = json::parse(f);
	} catch (const json::parse_error& e) {
		DBG_PRINT("JSON parse error: " << e.what() << endl);
		config.error = true;
		return;
	}

	if (doc.contains("port")) {
		string s = doc["port"];
		config.portName = Utils::strToWstr(s);
	}
	else
		config.portName = L"COM1";

	if (doc.contains("baud"))
		config.baudRate = doc["baud"];
	else
		config.baudRate = 115200;

	if (doc.contains("parity")) {
		string parity = doc["parity"];
		if (parity == "EVEN")
			config.parity = EVENPARITY;
		else if (parity == "ODD")
			config.parity = ODDPARITY;
		else
			config.parity = NOPARITY;
	}
	else
		config.parity = NOPARITY;

	if (doc.contains("device")) {
		string dev = doc["device"];
		config.deviceName = Utils::strToWstr(dev);
	}
	else
		config.deviceName = L"default";

	if (doc.contains("channels")) {
		json cfgChannels = doc["channels"];
		for (auto ch : cfgChannels) {
			int id = ch["id"];
			if (id >= 0 && id <= 5) {
				for (string strSession : ch["sessions"]) {
					config.channels[id].push_back(Utils::strToWstr(strSession));
				}

			}
		}
	}

	config.doc = doc;
}

void writeConfig() {
	json doc;
	doc["port"] = Utils::wStrToStr(config.portName);
	doc["baud"] = (int)config.baudRate;
	if (config.parity == EVENPARITY)
		doc["parity"] = "EVEN";
	else if (config.parity == ODDPARITY)
		doc["parity"] = "ODD";
	else
		doc["parity"] = "NONE";
	doc["device"] = Utils::wStrToStr(config.deviceName);

	json channels = json::array();
	for (int i = 0; i < config.channels.size(); i++) {
		vector<wstring>& vecChannel = config.channels[i];
		json channel;
		channel["id"] = i;
		channel["sessions"] = json::array();
		for (wstring& wstrSessionName : vecChannel)
			channel["sessions"].push_back(Utils::wStrToStr(wstrSessionName));

		channels.push_back(channel);
	}

	doc["channels"] = channels;

	ofstream out(CONFIG_FILE);
	out << doc.dump() << endl;
	out.close();
}

void cleanup() {
    delete io;
    delete vol;
}

void parsePipeMessage(string msg) {
	json doc;
	try {
		doc = json::parse(msg);
	}
	catch (const json::parse_error& e) {
		DBG_PRINT("JSON parse error: " << e.what() << endl);
		return;
	}
	if (doc.contains("request")) {
		string type = doc["request"];
		if (type == "config") {
			json data = json();
			data["configFile"] = config.doc;
			io->sendPipe(data.dump());
		}
	}
}

void parseMessage(string msg) {
	if (msg.size() == 0) return;
	if (msg == "READY") {
		// device was just connected
		controller->update(io);
		return;
	}
	if (msg[0] == '!') {
		// command
		if (msg.size() >= 4) {
			char cmd = msg[1];
			if (msg[2] == ':') {
				// all values
				vector<int> values = Utils::parseCmdAllVals(msg);
				switch (cmd) {
				case 'V':
					for (int i = 0; i < 6; i++) {
						int value = values[i];
						if (value >= 0 && value <= 100) {
							DBG_PRINT("Channel " << i << ": " << value << "%" << endl);
							controller->adjustVolume(i, value);
						}
					}
					break;
				case 'M':
					for (int i = 0; i < 6; i++) {
						int value = values[i];
						if (value >= 0 && value <= 1) {
							DBG_PRINT("Channel " << i << ": " << (value ? "mute" : "unmute") << endl);
							controller->adjustMute(i, value);
						}
					}
					break;
				}
			}
			else {
				// one value
				int chID;
				int value = Utils::parseCmd1Val(msg, chID);
				switch (cmd) {
				case 'V':
					if (chID >= 0 && chID <= 5 && value >= 0 && value <= 100) {
						DBG_PRINT("Channel " << chID << ": " << value << "%" << endl);
						controller->adjustVolume(chID, value);
					}
					break;
				case 'M':
					if (chID >= 0 && chID <= 5 && (value == 0 || value == 1)) {
						DBG_PRINT("Channel " << chID << ": " << (value ? "mute" : "unmute") << endl);
						controller->adjustMute(chID, value ? true : false);
					}
				}
			}
		}
	}
	else if (msg[0] == '?') {
		// request
		if (msg.size() >= 2) {
			char cmd = msg[1];
			switch (cmd) {
			case 'A':
				// channel active data
				io->send(controller->makeActiveDataCmd());
				break;
			case 'M':
				// mute data
				io->send(controller->makeMuteCmd());
				break;
			case 'V':
				// volume data
				io->send(controller->makeVolumeCmd());
			}
		}
	}
}

void ShowTrayMenu(HWND hWnd) {
	// Create the tray menu if it doesn't exist
	if (!hTrayMenu) {
		hTrayMenu = CreatePopupMenu();
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_OPEN_GUI, L"Open Settings");
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_RECONNECT, L"Reconnect");
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_RESCAN_SESSIONS, L"Rescan Audio Sessions");
		AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
	}

	POINT pt;
	GetCursorPos(&pt);
	SetForegroundWindow(hWnd);
	TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y-15, 0, hWnd, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		DBG_PRINT("WM_DESTROY" << endl);
		PostQuitMessage(0);
		break;
	case MSG_NOTIFYICON_CLICKED:
		if (lParam == WM_RBUTTONUP) {
			ShowTrayMenu(hWnd);
		}
		else if (lParam == WM_LBUTTONUP) {
			// TODO open GUI
			MessageBox(hWnd, L"This will open GUI", L"Volume Controller", MB_OK);
		}
		break;
	case MSG_DATAARRIVED:
		while (io->hasMessages()) {
			parseMessage(io->popMessage());
		}
		break;
	case MSG_PIPE_DATAARRIVED:
		while (io->hasPipeMessages()) {
			parsePipeMessage(io->popPipeMessage());
		}
		break;
	case MSG_CONNECTSUCCESS:
		break;
	case MSG_SESSION_DISCOVERED:
		controller->mapChannels();
		controller->update(io);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_TRAY_OPEN_GUI:
			// TODO open GUI
			MessageBox(hWnd, L"This will open GUI", L"Volume Controller", MB_OK);
			break;
		case ID_TRAY_RECONNECT:
			io->initSerialPort();
			break;
		case ID_TRAY_RESCAN_SESSIONS:
			controller->mapChannels();
			controller->update(io);
			break;
		case ID_TRAY_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	CreateMutex(NULL, TRUE, UNIQUE_MUTEX_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return 0;
	}
#ifdef DEBUG
    makeConsole();
#endif

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc,
					  0, 0, hInstance, NULL, NULL, NULL, NULL,
					  L"VolumeController_class", NULL };
	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(0, L"VolumeController_class", L"VolumeController Window",
		0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

	SHSTOCKICONINFO stockIconInfo;
	stockIconInfo.cbSize = sizeof(stockIconInfo);
	SHGetStockIconInfo(SIID_DESKTOPPC, SHGSI_ICON, &stockIconInfo);
	HICON hIconDisconnected = stockIconInfo.hIcon;
	SHGetStockIconInfo(SIID_NETWORKCONNECT, SHGSI_ICON, &stockIconInfo);
	HICON hIconConnected = stockIconInfo.hIcon;

	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_NOTIFYICON_CLICKED;
	nid.hIcon = hIconDisconnected;
	lstrcpy(nid.szTip, L"VolumeController");
	Shell_NotifyIcon(NIM_ADD, &nid);

	readConfig();
	writeConfig();

    vol = new VolumeManager();
    io = new IO(hWnd, &config);

    if (!vol->initialize(hWnd)) {
        cleanup();
        return 0;
    }
	vol->scanOutputDevices();
	if (config.deviceName == L"default") {
		vol->useDefaultOutputDevice();
	}
	else {
		vol->useOutputDevice(config.deviceName);
	}
	controller = new Controller(vol, &config);
	controller->mapChannels();

	io->initSerialPort();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &nid);
    cleanup();
    return 0;
}

void makeConsole() {
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
