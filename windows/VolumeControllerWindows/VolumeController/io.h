#pragma once
#include <Windows.h>
#include <string>
#include <queue>
#include <mutex>

#include "structs.h"
#include "globals.h"

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
	static DWORD WINAPI threadPipeRoutineStatic(LPVOID lpParam);
	void threadRoutine();
	void threadPipeRoutine();

	bool waitForPipeClient(OVERLAPPED& overlapped);

	Config* config;
	HANDLE hSerialPort;
	bool serialConnected;
	HANDLE hPipe;

	HANDLE hThread;
	HANDLE hThreadPipe;
	bool stopThread;
	bool stopThreadPipe;

	HWND hWnd;
	std::mutex qMutex;
	std::queue<std::string> messages;

	LPCWSTR PIPE_PATH = L"\\\\.\\pipe\\" PIPE_NAME;
};