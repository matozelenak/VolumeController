#include "utils.h"
#include <iostream>
#include <string>

#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <atlbase.h>
#include <mmdeviceapi.h>

using namespace std;

wstring Utils::getProcessName(DWORD pid) {
	wstring filename = L"";
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess) {
		WCHAR processName[MAX_PATH];
		if (GetModuleFileNameExW(hProcess, NULL, processName, MAX_PATH)) {
			filename = PathFindFileNameW(processName);
		}
		else {
			cerr << " (GetModuleFileName) error: " << GetLastError() << endl;
		}

		CloseHandle(hProcess);
	}
	else {
		cerr << "error (OpenProcess): " << GetLastError() << ", pid: " << pid << endl;
	}

	return filename;
}

wstring Utils::getIMMDeviceProperty(CComPtr<IMMDevice> device, const PROPERTYKEY& key) {
	CComPtr<IPropertyStore> props;
	device->OpenPropertyStore(STGM_READ, &props);

	PROPVARIANT prop;
	PropVariantInit(&prop);
	props->GetValue(key, &prop);

	if (prop.vt != VT_EMPTY) {
		wstring ret = prop.pwszVal;
		PropVariantClear(&prop);
		props.Release();
		return ret;
	}

	PropVariantClear(&prop);
	props.Release();
	return wstring();
}

wstring Utils::printLastError(wstring functionName) {
	wstring result = L"[Error] ";
	result += functionName;
	result += L": ";
	LPVOID buff;
	DWORD size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, (LPWSTR)&buff, 0, NULL);
	if (size > 0) {
		result += (LPCWSTR) buff;
	}
	else {
		result += L"unknown error";
	}
	if (buff) {
		LocalFree(buff);
	}
	wcerr << result << endl;
	return result;
}