#pragma once
#include <string>
#include <codecvt>

#include <Windows.h>
#include <Psapi.h>
#include <atlbase.h>
#include <mmdeviceapi.h>

class Utils {

public:
	static std::wstring getProcessName(DWORD pid);
	static std::wstring getIMMDeviceProperty(CComPtr<IMMDevice> device, const PROPERTYKEY& key);
	static std::wstring printLastError(std::wstring functionName);
	static std::wstring strToWstr(std::string& str);
	static std::string wStrToStr(std::wstring& wstr);

private:
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
};