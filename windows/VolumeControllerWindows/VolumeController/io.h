#pragma once
#include <Windows.h>
#include <string>
#include <queue>
#include <mutex>

#include "globals.h"
#include "structs.h"

class IO {
	
public:
	IO(HWND hWnd, Config* config);
	~IO();

	bool initSerialPort();
	void closeSerialPort();
	bool isSerialConnected();
	void cleanup();

	bool hasMessages();
	std::string popMessage();
	void send(std::string data);

	bool hasPipeMessages();
	std::string popPipeMessage();
	void sendPipe(std::string data);

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
	bool pipeConnected;

	HANDLE hThread;
	HANDLE hThreadPipe;
	bool stopThread;
	bool stopThreadPipe;

	HWND hWnd;
	std::mutex qMutex;
	std::queue<std::string> messages;
	std::mutex qPipeMutex;
	std::queue<std::string> pipeMessages;


	LPCWSTR PIPE_PATH = L"\\\\.\\pipe\\" PIPE_NAME;
};