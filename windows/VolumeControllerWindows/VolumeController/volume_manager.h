#pragma once

#include "structs.h"
#include "session_notification.h"
#include "session_interface.h"
#include <string>
#include <vector>
#include <memory>

#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>

class Controller;

class VolumeManager {

public:
	VolumeManager(Controller* controller);
	~VolumeManager();

	bool initialize(HWND hWnd);
	void scanDevicesAndSessions();
	
	std::vector<PAudioSession>& getSessionPool();
	std::vector<PAudioDevice>& getDevicePool();

	std::vector<PAudioSession>* getSessionPoolAddr();
	std::vector<PAudioDevice>* getDevicePoolAddr();

	std::vector<PISession> getSession(std::wstring name);
	PAudioDevice getDefaultOutputDevice();

	void disposeSessionsAndDevices();
	CComPtr<IMMDeviceEnumerator> getDeviceEnumerator();

	void cleanup();

private:
	void scanDeviceSessions(PAudioDevice device);

	std::vector<PAudioSession> _sessionPool;
	std::vector<PAudioDevice> _devicePool;

	CComPtr<IMMDeviceEnumerator> _deviceEnumerator;
	CComPtr<IMMDeviceCollection> _outputDevices;

	Controller* controller;

};