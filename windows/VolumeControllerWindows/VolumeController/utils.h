#pragma once
#include <string>

#include <Windows.h>
#include <Psapi.h>

class Utils {

public:
	static std::wstring getProcessName(DWORD pid);
};