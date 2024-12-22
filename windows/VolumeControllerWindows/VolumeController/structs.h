#pragma once

#include <string>
#include <vector>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <atlbase.h>
#include "json/json.hpp"

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
	boolean error = false;
	std::wstring portName = L"";
	DWORD baudRate = 115200;
	BYTE parity = EVENPARITY;
	std::wstring deviceName = L"default";
	std::vector<std::vector<std::wstring>> channels;
	nlohmann::json doc;
};