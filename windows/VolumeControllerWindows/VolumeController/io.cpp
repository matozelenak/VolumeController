#include "globals.h"
#include "io.h"
#include "utils.h"
#include <string>
#include <iostream>

#include <Windows.h>
using namespace std;
#define LOCK_QUEUE() lock_guard<mutex> lock(qMutex)
#define LOCK_PIPE_QUEUE() lock_guard<mutex> lockPipe(qPipeMutex)


IO::IO(HWND hWnd, Config* config) {
	this->config = config;
	hSerialPort = NULL;
	hPipe = NULL;
	serialConnected = false;
	hThread = NULL;
	hThreadPipe = NULL;
	stopThread = false;
	stopThreadPipe = false;
	pipeConnected = false;
	stopSerialRead = false;
	this->hWnd = hWnd;

	hThread = CreateThread(NULL, 0, threadRoutineStatic, this, 0, NULL);
	if (!hThread) {
		Utils::printLastError(L"CreateThread");
		return;
	}

	hThreadPipe = CreateThread(NULL, 0, threadPipeRoutineStatic, this, 0, NULL);
	if (!hThreadPipe) {
		Utils::printLastError(L"CreateThread");
		return;
	}
}

IO::~IO() {
	cleanup();
}

bool IO::initSerialPort() {
	if (serialConnected) return false;
	while (!messages.empty()) messages.pop();
	this->portName = config->portName;
	this->baudRate = config->baudRate;
	this->comParity = config->parity;

	DBG_PRINTW(L"opening serial port " << portName << L", baud: " << baudRate << endl);
	wstring portPath = L"\\\\.\\";
	portPath += portName;
	hSerialPort = CreateFile(portPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hSerialPort == INVALID_HANDLE_VALUE) {
		Utils::printLastError(L"CreateFile");
		return false;
	}

	if (!setCommParameters()) {
		closeSerialPort();
		return false;
	}

	serialConnected = true;
	PostMessage(hWnd, MSG_SERIAL_CONNECTED, 0, 0);
	return true;
}

void IO::closeSerialPort() {
	if (!serialConnected) return;
	DBG_PRINTW(L"closing serial port " << portName << endl);
	CloseHandle(hSerialPort);
	stopSerialRead = false;
	serialConnected = false;
	PostMessage(hWnd, MSG_SERIAL_DISCONNECTED, 0, 0);
}

void IO::disconnectSerialPort() {
	if (!serialConnected) return;
	stopSerialRead = true;
	DBG_PRINT("disconnecting serial port..." << endl);
	while (serialConnected) Sleep(100);
}

bool IO::isSerialConnected() {
	return serialConnected;
}

void IO::cleanup() {
	if (hThread) {
		stopThread = true;
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = NULL;
	}
	closeSerialPort();
	if (hThreadPipe) {
		stopThreadPipe = true;
		WaitForSingleObject(hThreadPipe, INFINITE);
		CloseHandle(hThreadPipe);
		hThreadPipe = NULL;
	}
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
			while (!stopSerialRead && pointer < 99) {
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
				PostMessage(hWnd, MSG_SERIAL_DATAARRIVED, 0, 0);
			}

			if (stopSerialRead)
				closeSerialPort();
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

DWORD WINAPI IO::threadPipeRoutineStatic(LPVOID lpParam) {
	IO* instance = static_cast<IO*>(lpParam);
	instance->threadPipeRoutine();
	return 0;
}

void IO::threadPipeRoutine() {
	while (!stopThreadPipe) {
		pipeConnected = false;
		OVERLAPPED olConnect = { 0 };
		DBG_PRINT("Creating named pipe..." << endl);
		hPipe = CreateNamedPipe(
			PIPE_PATH, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			1, 1024, 1024, 0, nullptr);

		if (hPipe == INVALID_HANDLE_VALUE) {
			Utils::printLastError(L"CreateNamedPipe");
			return;
		}

		olConnect.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (olConnect.hEvent == INVALID_HANDLE_VALUE) {
			Utils::printLastError(L"CreateEvent");
			CloseHandle(hPipe);
			return;
		}

		// wait for a client to connect
		pipeConnected = waitForPipeClient(olConnect);
		if (!pipeConnected) {
			CloseHandle(hPipe);
			hPipe = NULL;
			continue;
		}

		// client successfully connected
		while (!stopThreadPipe) {
			OVERLAPPED olRead = { 0 };
			olRead.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (olRead.hEvent == INVALID_HANDLE_VALUE) {
				Utils::printLastError(L"CreateEvent");
				break;
			}
			char buffer[1024];
			DWORD bytesRead = 0;

			BOOL success = ReadFile(hPipe, buffer, sizeof(buffer)-1, &bytesRead, &olRead);
			if (!success) {
				DWORD error = GetLastError();
				if (error == ERROR_IO_PENDING) {
					DWORD waitResult;
					do {
						waitResult = WaitForSingleObject(olRead.hEvent, 1000);
						switch (waitResult) {
						case WAIT_OBJECT_0:
							if (GetOverlappedResult(hPipe, &olRead, &bytesRead, FALSE)) {
								buffer[bytesRead] = '\0';
								DBG_PRINT(bytesRead << " Received: '" << buffer << "'" << endl);
								LOCK_PIPE_QUEUE();
								pipeMessages.push(buffer);
								PostMessage(hWnd, MSG_PIPE_DATAARRIVED, 0, 0);
							}
							else {
								Utils::printLastError(L"GetOverlappedResult");
							}
							break;
						case WAIT_TIMEOUT:
							//DBG_PRINT("Pipe read timeout" << endl); // TODO remove
							break;
						default:
							Utils::printLastError(L"WaitForSingleObject");
							break;
						}

					} while (waitResult == WAIT_TIMEOUT && !stopThreadPipe);
				}
				else if (error == ERROR_BROKEN_PIPE) {
					DBG_PRINT("Pipe client disconnected." << endl);
					break;
				}
				else {
					Utils::printLastError(L"ReadFile");
					break;
				}
				
			}
			else {
				// ReadFile completed immediatelly
				buffer[bytesRead] = '\0';
				DBG_PRINT(bytesRead << " Immediatelly Received: '" << buffer << "'" << endl);
				LOCK_PIPE_QUEUE();
				pipeMessages.push(buffer);
				PostMessage(hWnd, MSG_PIPE_DATAARRIVED, 0, 0);
			}

			CloseHandle(olRead.hEvent);

		} // ReadFile loop

		DBG_PRINT("Closing pipe..." << endl);
		CloseHandle(hPipe);
		pipeConnected = false;

	} // CreateNamedPipe loop
}

bool IO::waitForPipeClient(OVERLAPPED& overlapped) {
	bool result = false;
	BOOL connected = ConnectNamedPipe(hPipe, &overlapped);
	if (!connected) {
		DWORD error = GetLastError();
		if (error == ERROR_IO_PENDING) {
			DWORD waitResult;
			do {
				// wait for connection
				waitResult = WaitForSingleObject(overlapped.hEvent, 1000);
				if (waitResult == WAIT_OBJECT_0) {
					result = true;
				}
				else if (waitResult == WAIT_TIMEOUT) {
					//DBG_PRINT("Client wait timeout" << endl); // TODO REMOVE
				}
				else {
					Utils::printLastError(L"WaitForSingleObject");
					break;
				}

			} while (waitResult != WAIT_OBJECT_0 && !stopThreadPipe);
		}
		else if (error == ERROR_PIPE_CONNECTED) {
			result = true;
		}
		else {
			Utils::printLastError(L"ConnectNamedPipe");
		}
	}
	else {
		result = true;
	}

	CloseHandle(overlapped.hEvent);

	return result;
}

bool IO::hasPipeMessages() {
	LOCK_PIPE_QUEUE();
	return pipeMessages.size() > 0;
}

string IO::popPipeMessage() {
	LOCK_PIPE_QUEUE();
	if (pipeMessages.size() == 0) return string();
	string msg = pipeMessages.front();
	pipeMessages.pop();
	return msg;
}

void IO::sendPipe(string data) {
	if (!pipeConnected) return;

	OVERLAPPED olWrite = { 0 };
	olWrite.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (olWrite.hEvent == INVALID_HANDLE_VALUE) {
		Utils::printLastError(L"CreateEvent");
		return;
	}

	DWORD bytesWritten;
	BOOL w = WriteFile(hPipe, data.c_str(), data.size(), &bytesWritten, &olWrite);
	if (!w) {
		DWORD error = GetLastError();
		if (error == ERROR_IO_PENDING) {
			DWORD waitResult;
			do {
				// wait
				waitResult = WaitForSingleObject(olWrite.hEvent, 1000);
				if (waitResult == WAIT_OBJECT_0) {
					// success
					cout << "[TXpipe]: '" << data << "'" << endl;
				}
				else if (waitResult == WAIT_TIMEOUT) {
					
				}
				else {
					Utils::printLastError(L"WaitForSingleObject");
					break;
				}

			} while (waitResult != WAIT_OBJECT_0 && !stopThreadPipe);
		}
		else if (error == ERROR_BROKEN_PIPE) {
			DBG_PRINT("Write failed: pipe client disconnected." << endl);
			Utils::printLastError(L"WriteFile");
		}
		else {
			Utils::printLastError(L"WriteFile");
		}
	}
	else if (bytesWritten == data.size()) {
		// success
		cout << "[TXpipe]: '" << data << "'" << endl;
	}
	else {
		Utils::printLastError(L"WriteFile");
	}

	CloseHandle(olWrite.hEvent);

}

bool IO::isPipeConnected() {
	return pipeConnected;
}

void IO::configChanged() {
	if (config->portName != portName || config->baudRate != baudRate || config->parity != comParity) {
		closeSerialPort();
		initSerialPort();
	}
}
