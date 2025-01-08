#pragma once
#include <memory>

#define MSG_NOTIFYICON_CLICKED (WM_USER+1)
#define MSG_SERIAL_DATAARRIVED (WM_USER+2)
#define MSG_SERIAL_CONNECTED (WM_USER+3)
#define MSG_SERIAL_DISCONNECTED (WM_USER+4)
#define MSG_PIPE_DATAARRIVED (WM_USER+5)
#define MSG_SESSION_DESTROYED (WM_USER+6)
#define MSG_SESSION_DISCOVERED (WM_USER+7)

#define ID_TRAY_OPEN_GUI 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAY_RECONNECT 1003
#define ID_TRAY_RESCAN_SESSIONS 1004
#define ID_TRAY_DEBUGINFO 1005

#define PIPE_NAME L"VolumeControllerPipe"
#define UNIQUE_MUTEX_NAME L"VolumeControllerMutex"
#define TRAYICON_TIP L"Volume Controller"
#define WND_CLASS L"VolumeController_class"
#define WND_TITLE L"VolumeController Window"
#define GUI_EXE_PATH L"volumecontroller-gui-win32-x64\\volumecontroller-gui.exe"

#ifdef _DEBUG
	#define DBG_PRINT(msg) std::cout << msg;
	#define DBG_PRINTW(msg) std::wcout << msg;
#else
	#define DBG_PRINT(msg)
	#define DBG_PRINTW(msg)
#endif

class VolumeManager;
class Controller;
class ISession;
class AudioSession;
class AudioDevice;
class IO;
class SessionCreatedNotification;
class AudioDeviceNotification;
class AudioSessionEvents;
class Channel;
class Utils;

struct Config;

typedef std::shared_ptr<ISession> PISession;
typedef std::shared_ptr<AudioDevice> PAudioDevice;
typedef std::shared_ptr<AudioSession> PAudioSession;
