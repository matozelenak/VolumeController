#pragma once

#include <string>
#include <vector>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <atlbase.h>

struct AudioSession {
	bool isSystemSounds;
	DWORD pid;
	std::wstring filename;
	CComPtr<ISimpleAudioVolume> volume;
};

struct InputDevice {
	std::wstring friedlyName;
	CComPtr<IAudioEndpointVolume> volume;
};

struct Channel {
	int id;
	std::vector<std::wstring> sessions;
};

struct Config {
	boolean error;
	std::wstring portName;
	DWORD baudRate;
	BYTE parity;
	std::wstring deviceName;
	std::vector<std::vector<std::wstring>> channels;
};