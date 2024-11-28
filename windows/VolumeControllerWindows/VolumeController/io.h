#pragma once
#include <Windows.h>
#include <string>
#include <queue>
#include <mutex>

#include "structs.h"

class IO {
	
public:
	IO(HWND hWnd, Config* config);
	~IO();

	bool initSerialPort();
	void closeSerialPort();
	void cleanup();

	bool hasMessages();
	std::string popMessage();
	void send(std::string data);

private:
	bool setCommParameters();
	static DWORD WINAPI threadRoutineStatic(LPVOID lpParam);
	void threadRoutine();

	Config* config;
	HANDLE hSerialPort;
	bool serialConnected;

	HANDLE hThread;
	bool stopThread;

	HWND hWnd;
	std::mutex qMutex;
	std::queue<std::string> messages;
};