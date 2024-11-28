#include "io.h"
#include "utils.h"
#include "globals.h"
#include <string>
#include <iostream>

#include <Windows.h>
using namespace std;
#define LOCK_QUEUE() lock_guard<mutex> lock(qMutex)

IO::IO(HWND hWnd, Config* config) {
	this->config = config;
	hSerialPort = NULL;
	serialConnected = false;
	hThread = NULL;
	stopThread = false;
	this->hWnd = hWnd;

	hThread = CreateThread(NULL, 0, threadRoutineStatic, this, 0, NULL);
	if (!hThread) {
		Utils::printLastError(L"CreateThread");
	}
}

IO::~IO() {
	cleanup();
}

bool IO::initSerialPort() {
	if (serialConnected) return false;
	while (!messages.empty()) messages.pop();
	wcout << L"opening serial port " << config->portName << L", baud: " << config->baudRate << endl;
	hSerialPort = CreateFile(config->portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hSerialPort == INVALID_HANDLE_VALUE) {
		Utils::printLastError(L"CreateFile");
		return false;
	}

	if (!setCommParameters()) {
		return false;
	}

	serialConnected = true;
	PostMessage(hWnd, MSG_CONNECTSUCCESS, 0, 0);
	return true;
}

void IO::closeSerialPort() {
	if (!serialConnected) return;
	CloseHandle(hSerialPort);
	serialConnected = false;
	cout << "closing serial port";
}

void IO::cleanup() {
	if (hThread) {
		stopThread = true;
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = NULL;
	}
	closeSerialPort();
}



bool IO::setCommParameters() {
	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = config->baudRate;
	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = FALSE;
	dcb.ByteSize = 8;
	dcb.Parity = config->parity;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState(hSerialPort, &dcb)) {
		Utils::printLastError(L"SetCommState");
		CloseHandle(hSerialPort);
		return false;
	}

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 100;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerialPort, &timeouts)) {
		Utils::printLastError(L"SetCommTimeouts");
		CloseHandle(hSerialPort);
		return false;
	}

	return true;
}


DWORD WINAPI IO::threadRoutineStatic(LPVOID lpParam) {
	IO* instance = static_cast<IO*>(lpParam);
	instance->threadRoutine();
	return 0;
}

void IO::threadRoutine() {
	while (!stopThread) {
		// if serial port not connected
		if (!serialConnected) {
			Sleep(1000);
		}
		// if serial port connected
		else {
			char buffer[100];
			int pointer = 0;
			char c[1];
			DWORD bytesRead;
			while (pointer < 99) {
				BOOL r = ReadFile(hSerialPort, c, 1, &bytesRead, NULL);
				if (!r) {
					Utils::printLastError(L"ReadFile");
					closeSerialPort();
					break;
				}
				if (bytesRead == 0) { // timeout
					break;
				}
				else {
					if (c[0] == '\n') { // newline
						break;
					}
					if (c[0] >= 0 && c[0] <= 127 && !isspace(c[0])) {
						buffer[pointer] = c[0];
						pointer++;
					}
				}
			}
			buffer[pointer] = '\0';
			
			if (pointer > 0) {
				// data was received
				LOCK_QUEUE();
				messages.push(buffer);
				cout << "[RX]: '" << buffer << "'" << endl;
				pointer = 0;
				PostMessage(hWnd, MSG_DATAARRIVED, 0, 0);
			}

		}
	}
}

bool IO::hasMessages() {
	LOCK_QUEUE();
	return messages.size() > 0;
}

string IO::popMessage() {
	LOCK_QUEUE();
	if (messages.size() == 0) return string();
	string msg = messages.front();
	messages.pop();
	return msg;
}

void IO::send(string data) {
	if (!serialConnected) return;
	DWORD bytesWritten;
	BOOL w = WriteFile(hSerialPort, data.c_str(), data.size(), &bytesWritten, NULL);
	if (!w) {
		Utils::printLastError(L"WriteFile");
		closeSerialPort();
	}
	else if (bytesWritten == data.size()) {
		// success
		cout << "[TX]: '" << data << "'" << endl;
	}
	else {
		Utils::printLastError(L"WriteFile");
		closeSerialPort();
	}

}