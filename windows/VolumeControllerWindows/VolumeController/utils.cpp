#include "utils.h"
#include <iostream>
#include <string>

#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>

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