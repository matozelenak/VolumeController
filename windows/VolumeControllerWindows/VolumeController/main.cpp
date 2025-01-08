#include "volume_manager.h"
#include "io.h"
#include "controller.h"
#include "utils.h"
#include "structs.h"
#include "session_notification.h"
#include "globals.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <Windows.h>
#include <shellapi.h>
#include <fstream>
#include <codecvt>
#include <Dbt.h>
#include "json/json.hpp"

#pragma comment(lib, "OneCore.lib")

using json = nlohmann::json;
using namespace std;

const char* CONFIG_FILE = "config.json";

VolumeManager* vol = nullptr;
IO* io = nullptr;
Controller* controller = nullptr;
Config config;

HWND hWnd = NULL;
HMENU hTrayMenu = NULL;
HDEVNOTIFY hDeviceNotify = NULL;
HICON hIconConnected = NULL;
HICON hIconDisconnected = NULL;
NOTIFYICONDATA nid;

void makeStatusJSON(json& doc) {
	doc["status"] = json();
	doc["status"]["deviceConnected"] = io->isSerialConnected();
	ULONG ports[20];
	ULONG count = 20;
	ULONG found;
	GetCommPorts(ports, count, &found);
	doc["status"]["comPorts"] = json::array();
	DBG_PRINT("found " << found << " com ports" << endl);
	for (int i = 0; i < found && i < count; i++) {
		stringstream portName;
		portName << "COM";
		portName << (int)ports[i];
		doc["status"]["comPorts"].push_back(portName.str());
		DBG_PRINT("  found " << portName.str() << endl);
	}
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
		json response = json();
		json requests = doc["request"];
		for (string type : requests) {
			if (type == "status") {
				makeStatusJSON(response);
			}
			else if (type == "config") {
				response["configFile"] = Utils::storeConfigToJSON(config);
			}
			else if (type == "sessionPool") {
				json sessionPool = json::array();
				for (PAudioSession& session : vol->getSessionPool()) {
					wstring sessionName = session->getName();
					sessionPool.push_back(Utils::wStrToStr(sessionName));
				}
				response["sessionPool"] = sessionPool;
			}
			else if (type == "devicePool") {
				json devicePool = json::array();
				for (PAudioDevice& device : vol->getDevicePool()) {
					wstring deviceName = device->getName();
					devicePool.push_back(Utils::wStrToStr(deviceName));
				}
				response["devicePool"] = devicePool;
			}
		}
		io->sendPipe(response.dump());
	}
	if (doc.contains("configFile")) {
		// save config
		json cfg = doc["configFile"];
		Utils::parseConfigFromJSON(cfg, config);
		json doc = Utils::storeConfigToJSON(config);
		Utils::writeConfig(doc, CONFIG_FILE);

		io->configChanged();
		controller->rescanAndRemap();
		controller->update();
	}
}

void parseMessage(string msg) {
	if (msg.size() == 0) return;
	if (msg == "READY") {
		// device was just connected
		controller->update();
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

void registerComDeviceNotification() {
	const GUID GUID_DEVINTERFACE_COMPORT = { 0x86E0D1E0L, 0x8089, 0x11D0, { 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73 } };

	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = {};
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_COMPORT;

	hDeviceNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (hDeviceNotify == NULL) {
		DBG_PRINT("Failed to register device notification" << endl);
	}
}

void unregisterComDeviceNotification() {
	if (hDeviceNotify != NULL) {
		UnregisterDeviceNotification(hDeviceNotify);
		hDeviceNotify = NULL;
	}
}

void ShowTrayMenu(HWND hWnd) {
	if (!hTrayMenu) {
		hTrayMenu = CreatePopupMenu();
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_OPEN_GUI, L"Open Settings");
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_RECONNECT, L"Reconnect");
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_RESCAN_SESSIONS, L"Rescan Audio Sessions");
		AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_DEBUGINFO, L"Debug info");
		AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
	}

	// show the menu
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

	case MSG_SERIAL_DATAARRIVED:
		// data arrived on serial port
		while (io->hasMessages()) {
			parseMessage(io->popMessage());
		}
		break;

	case MSG_SERIAL_CONNECTED:
		// COM device was connected successfully
		if (io->isPipeConnected()) {
			json doc;
			makeStatusJSON(doc);
			io->sendPipe(doc.dump());
		}
		nid.hIcon = hIconConnected;
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		break;

	case MSG_SERIAL_DISCONNECTED:
		// COM device was disconnected
		if (io->isPipeConnected()) {
			json doc;
			makeStatusJSON(doc);
			io->sendPipe(doc.dump());
		}
		nid.hIcon = hIconDisconnected;
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		break;

	case MSG_PIPE_DATAARRIVED:
		// pipe data arrived
		while (io->hasPipeMessages()) {
			parsePipeMessage(io->popPipeMessage());
		}
		break;

	case MSG_SESSION_DESTROYED:
		// when an audio session was destroyed
		controller->sessionDestroyed();
		break;

	case MSG_NOTIFYICON_CLICKED:
		// when the tray icon was clicked
		if (lParam == WM_RBUTTONUP) {
			ShowTrayMenu(hWnd);
		}
		else if (lParam == WM_LBUTTONUP) {
			Utils::openGUI();
		}
		break;

	case WM_COMMAND:
		// when tray menu item is clicked
		switch (LOWORD(wParam)) {
		case ID_TRAY_OPEN_GUI:
			Utils::openGUI();
			break;
		case ID_TRAY_RECONNECT:
			io->disconnectSerialPort();
			io->initSerialPort();
			break;
		case ID_TRAY_RESCAN_SESSIONS:
			controller->rescanAndRemap();
			controller->update();
			break;
		case ID_TRAY_DEBUGINFO:
			MessageBox(NULL, Utils::makeDebugInfo(vol, controller).c_str(), L"Debug Info", MB_OK);
			break;
		case ID_TRAY_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;

	case WM_DEVICECHANGE:
		// COM device connected or disconnected
		if (wParam == DBT_DEVICEARRIVAL) {
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
			if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
				DBG_PRINTW(L"new device connected: " << pDevInf->dbcc_name << endl);
				if (!io->isSerialConnected())
					io->initSerialPort();
			}
		}
		else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
			if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
				DBG_PRINTW(L"device removed: " << pDevInf->dbcc_name << endl);
			}
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// check if another instance of this program is running, of yes, exit
	CreateMutex(NULL, TRUE, UNIQUE_MUTEX_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return 0;
	}
#ifdef _DEBUG
    Utils::makeConsole();
#endif

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc,
					  0, 0, hInstance, NULL, NULL, NULL, NULL,
					  WND_CLASS, NULL };
	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(0, WND_CLASS, WND_TITLE,
		0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

	// TODO temporary tray icons
	SHSTOCKICONINFO stockIconInfo;
	stockIconInfo.cbSize = sizeof(stockIconInfo);
	SHGetStockIconInfo(SIID_DESKTOPPC, SHGSI_ICON, &stockIconInfo);
	hIconDisconnected = stockIconInfo.hIcon;
	SHGetStockIconInfo(SIID_NETWORKCONNECT, SHGSI_ICON, &stockIconInfo);
	hIconConnected = stockIconInfo.hIcon;

	// set up tray icon
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_NOTIFYICON_CLICKED;
	nid.hIcon = hIconDisconnected;
	lstrcpy(nid.szTip, TRAYICON_TIP);
	Shell_NotifyIcon(NIM_ADD, &nid);

	if (CoInitialize(NULL) != S_OK) {
		return -1;
	}

	json cfg = Utils::readConfig(CONFIG_FILE);
	Utils::parseConfigFromJSON(cfg, config);

    io = new IO(hWnd, &config);
	controller = new Controller(hWnd, &config, io);
	vol = controller->getVolumeManager();
	controller->rescanAndRemap();

	io->initSerialPort();

	registerComDeviceNotification();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	unregisterComDeviceNotification();

	Shell_NotifyIcon(NIM_DELETE, &nid);
	delete io;
	delete controller;
	CoUninitialize();
    return 0;
}

