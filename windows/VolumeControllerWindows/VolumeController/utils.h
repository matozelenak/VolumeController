#pragma once
#include <string>

#include <Windows.h>
#include <Psapi.h>
#include <atlbase.h>
#include <mmdeviceapi.h>

class Utils {

public:
	static std::wstring getProcessName(DWORD pid);
	static std::wstring getIMMDeviceProperty(CComPtr<IMMDevice> device, const PROPERTYKEY& key);
	static std::wstring printLastError(std::wstring functionName);
};