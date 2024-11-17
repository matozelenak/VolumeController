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
using namespace std;

void makeConsole();

VolumeManager* vol = nullptr;
IO* io = nullptr;
Controller* controller = nullptr;
wstring comPortName;
DWORD comPortBaud;

HWND hWnd = NULL;
HMENU hTrayMenu = NULL;

void cleanup() {
    delete io;
    delete vol;
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
			// adjust volume
			if (msg[1] == 'V') {
				if (msg[2] == ':') {

				}
				else {
					size_t colon = msg.find(':', 0);
					string strChID  = msg.substr(2, colon);
					int chID = stoi(strChID);
					string strValue = msg.substr(colon + 1, string::npos);
					int value = stoi(strValue);

					if (chID >= 0 && chID <= 5 && value >= 0 && value <= 100) {
						cout << "Channel " << chID << ": " << value << endl;
						controller->adjustVolume(chID, value);
					}
				}
			}
			// mute/unmute
			else if (msg[1] == 'M') {
				if (msg[2] == ':') {

				}
				else {
					size_t colon = msg.find(':', 0);
					string strChID = msg.substr(2, colon);
					int chID = stoi(strChID);
					string strValue = msg.substr(colon + 1, string::npos);
					int value = stoi(strValue);

					if (chID >= 0 && chID <= 5 && (value == 0 || value == 1)) {
						cout << "Channel " << chID << ": " << (value ? "mute" : "unmute") << endl;
						controller->adjustMute(chID, value ? true : false);
					}
				}
			}
		}
	}
	else if (msg[0] == '?') {
		// request
		if (msg.size() >= 2) {
			if (msg[1] == 'A') {
				// channel active data
				io->send(controller->makeActiveDataCmd());
			}
			else if (msg[1] == 'M') {
				// mute data
				io->send(controller->makeMuteCmd());
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
		cout << "WM_DESTROY" << endl;
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
			io->initSerialPort(comPortName, comPortBaud);
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
    makeConsole();

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



    vol = new VolumeManager();
    io = new IO(hWnd);
	comPortName = L"COM3";
	comPortBaud = CBR_115200;

    if (!vol->initialize(hWnd)) {
        cleanup();
        return 0;
    }
    vol->useDefaultOutputDevice();
	controller = new Controller(vol);
	controller->mapChannels();

	io->initSerialPort(comPortName, comPortBaud);

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
