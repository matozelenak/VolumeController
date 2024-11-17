#pragma once

#include "structs.h"
#include "session_notification.h"
#include <string>
#include <vector>

#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>

class VolumeManager {

public:
	VolumeManager();
	~VolumeManager();

	bool initialize(HWND hWnd);
	std::vector<std::wstring> scanOutputDevices();

	void useOutputDevice(std::wstring name);
	void useDefaultOutputDevice();
	void setOutputDeviceVolume(float volume);
	float getOutputDeviceVolume();
	void setOutputDeviceMute(bool mute);
	bool getOutputDeviceMute();
	
	std::vector<AudioSession> discoverSessions();
	void setSessionVolume(int index, float volume);
	void setSessionVolume(std::wstring name, float volume);
	float getSessionVolume(int index);
	float getSessionVolume(std::wstring name);
	void setSessionMute(int index, bool mute);
	void setSessionMute(std::wstring name, bool mute);
	bool getSessionMute(int index);
	bool getSessionMute(std::wstring name);

	void cleanup();

private:
	int sessionNameToIndex(std::wstring name);
	CComPtr<IMMDeviceEnumerator> deviceEnumerator;
	CComPtr<IMMDeviceCollection> outputDevices;
	std::vector<std::wstring> outputDevicesNames;
	CComPtr<IMMDevice> currentDevice;
	CComPtr<IAudioEndpointVolume> currentDeviceVolume;
	bool useDefOutDevice;
	std::wstring deviceNameToUse;
	void initCurrentOutputDevice();

	CComPtr<IAudioSessionManager2> sessionManager;
	std::vector<AudioSession> audioSessions;
	SessionCreatedNotification* sessionNotification;

	std::vector<InputDevice> inputDevices;
};