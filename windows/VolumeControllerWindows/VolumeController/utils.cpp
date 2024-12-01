#include "utils.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <locale>
#include <sstream>

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


wstring_convert<codecvt_utf8<wchar_t>> Utils::utf8_conv;

wstring Utils::strToWstr(string& str) {
	return utf8_conv.from_bytes(str);
}

string Utils::wStrToStr(wstring& wstr) {
	return utf8_conv.to_bytes(wstr);
}


string Utils::makeCmd1Val(char cmd, int ch, int val) {
	stringstream ss;
	ss << "!" << cmd << ch << ":" << ch << "\n";
	return ss.str();
}

string Utils::makeCmdAllVals(char cmd, vector<int>& vals) {
	stringstream ss;
	ss << "!" << cmd << ":";
	for (int i : vals)
		ss << i << "|";
	ss << "\n";
	return ss.str();
}

int Utils::parseCmd1Val(string& data, int& chID) {
	size_t colon = data.find(':', 0);
	string strChID = data.substr(2, colon);
	chID = stoi(strChID);
	string strValue = data.substr(colon + 1, string::npos);
	int value = stoi(strValue);
	return value;
}

vector<int> Utils::parseCmdAllVals(string& data) {
	vector<int> result;
	size_t begin = data.find(':') + 1;
	int pos = 0, prev = -1 + begin;
	while (1) {
		pos = data.find('|', prev + 1);
		if (pos == string::npos) break;
		string strVal = data.substr(prev + 1, pos);
		result.push_back(stoi(strVal));
		prev = pos;
	}
	return result;
}