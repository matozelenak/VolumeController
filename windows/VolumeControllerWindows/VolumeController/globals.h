#pragma once

#define MSG_NOTIFYICON_CLICKED (WM_USER+1)
#define MSG_DATAARRIVED (WM_USER+2)
#define MSG_CONNECTSUCCESS (WM_USER+3)
#define MSG_SESSION_DISCOVERED (WM_USER+4)

#define ID_TRAY_OPEN_GUI 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAY_RECONNECT 1003
#define ID_TRAY_RESCAN_SESSIONS 1004

#define PIPE_NAME L"VolumeControllerPipe"
#define UNIQUE_MUTEX_NAME L"VolumeControllerMutex"

#ifdef DEBUG
	#define DBG_PRINT(msg) std::cout << msg;
	#define DBG_PRINTW(MSG) std::wcout << msg;
#else
	#define DBG_PRINT(msg)
	#define DBG_PRINTW(msg)
#endif