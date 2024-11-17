#pragma once
#include <Windows.h>
#include <string>
#include <queue>
#include <mutex>

class IO {
	
public:
	IO(HWND hWnd);
	~IO();

	bool initSerialPort(std::wstring portName, DWORD baudRate);
	void closeSerialPort();
	void cleanup();

	bool hasMessages();
	std::string popMessage();
	void send(std::string data);

private:
	bool setCommParameters();
	static DWORD WINAPI threadRoutineStatic(LPVOID lpParam);
	void threadRoutine();

	std::wstring portName;
	DWORD baudRate;
	HANDLE hSerialPort;
	bool serialConnected;

	HANDLE hThread;
	bool stopThread;

	HWND hWnd;
	std::mutex qMutex;
	std::queue<std::string> messages;
};