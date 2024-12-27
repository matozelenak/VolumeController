#include "globals.h"
#include "volume_manager.h"
#include "session_interface.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <vector>

#include <atlbase.h> // CComPtr
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <Functiondiscoverykeys_devpkey.h> // device property keys

using namespace std;

VolumeManager::VolumeManager(Controller* controller) {
	this->controller = controller;
	//useDefOutDevice = false;
	//sessionNotification = nullptr;
	//deviceNameToUse = L"";
}

VolumeManager::~VolumeManager() {
	cleanup();
}

bool VolumeManager::initialize(HWND hWnd) {

	if (CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&_deviceEnumerator)) != S_OK) {
		return false;
	}

	//sessionNotification = new SessionCreatedNotification(hWnd);

	return true;
}

void VolumeManager::scanDevicesAndSessions() {
	DBG_PRINT("scanning devices and sessions..." << endl);
	disposeSessionsAndDevices();

	_deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &_outputDevices);

	UINT count;
	_outputDevices->GetCount(&count);

	for (UINT i = 0; i < count; i++) {
		CComPtr<IMMDevice> device;
		_outputDevices->Item(i, &device);
		_devicePool.push_back(make_shared<AudioDevice>(device, controller));
		scanDeviceSessions(_devicePool.back());
	}

}

vector<PAudioSession>& VolumeManager::getSessionPool() {
	return _sessionPool;
}

vector<PAudioDevice>& VolumeManager::getDevicePool() {
	return _devicePool;
}

vector<PAudioSession>* VolumeManager::getSessionPoolAddr() {
	return &_sessionPool;
}

vector<PAudioDevice>* VolumeManager::getDevicePoolAddr() {
	return &_devicePool;
}

vector<PISession> VolumeManager::getSession(wstring name) {
	vector<PISession> result;
	for (PAudioSession& session : _sessionPool) {
		if (session->getName() == name) result.push_back(session);
	}

	for (PAudioDevice& device : _devicePool) {
		if (device->getName() == name) result.push_back(device);
	}

	return result;
}

//AudioDevice* VolumeManager::getMasterOut() {
//	return _masterOut;
//}
PAudioDevice VolumeManager::getDefaultOutputDevice() {
	CComPtr<IMMDevice> device;
	_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
	wstring name = Utils::getIMMDeviceProperty(device, PKEY_Device_FriendlyName);

	vector<PISession> vec = getSession(name);
	if (vec.size() > 0)
		return static_pointer_cast<AudioDevice>(vec[0]);

	return PAudioDevice();
}

void VolumeManager::scanDeviceSessions(PAudioDevice device) {
	CComPtr<IAudioSessionEnumerator> sessionEnumerator = device->getSessionEnumerator();

	int count;
	sessionEnumerator->GetCount(&count);
	for (int i = 0; i < count; i++) {
		CComPtr<IAudioSessionControl> sessionControl;
		sessionEnumerator->GetSession(i, &sessionControl);
		_sessionPool.push_back(make_shared<AudioSession>(sessionControl, controller));
	}
}

void VolumeManager::disposeSessionsAndDevices() {
	_devicePool.clear();
	_sessionPool.clear();
	_outputDevices.Release();
}

CComPtr<IMMDeviceEnumerator> VolumeManager::getDeviceEnumerator() {
	return _deviceEnumerator;
}

//vector<wstring> VolumeManager::scanOutputDevices() {
//	outputDevicesNames.clear();
//	outputDevices.Release();
//	deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &outputDevices);
//	
//	UINT count;
//	outputDevices->GetCount(&count);
//	
//	for (UINT i = 0; i < count; i++) {
//		CComPtr<IMMDevice> device;
//		outputDevices->Item(i, &device);
//		
//		wstring devName = Utils::getIMMDeviceProperty(device, PKEY_Device_FriendlyName);
//		if (devName != L"") {
//			outputDevicesNames.push_back(devName);
//		}
//		else {
//			outputDevicesNames.push_back(wstring(L"error: device does not have a friendly name"));
//			MessageBox(NULL, L"device does not have a friendly name", L"VolumeController", 0);
//		}
//
//		device.Release();
//	}
//
//	if (useDefOutDevice)
//		useDefaultOutputDevice();
//	else if (deviceNameToUse != L"")
//		useOutputDevice(deviceNameToUse);
//	return outputDevicesNames;
//}
//
//void VolumeManager::useOutputDevice(wstring name) {
//	useDefOutDevice = false;
//	deviceNameToUse = name;
//	if (!outputDevices) return;
//	currentDevice.Release();
//
//	UINT count;
//	outputDevices->GetCount(&count);
//	for (int i = 0; i < count; i++) {
//		if (outputDevicesNames[i] == deviceNameToUse) {
//			outputDevices->Item(i, &currentDevice);
//			initCurrentOutputDevice();
//			wcout << L"using device: " << name << endl;
//			discoverSessions();
//			return;
//		}
//	}
//	wcout << L"device not found: " << name << endl;
//}
//
//void VolumeManager::useDefaultOutputDevice() {
//	useDefOutDevice = true;
//	currentDevice.Release();
//	deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &currentDevice);
//	initCurrentOutputDevice();
//	wcout << L"using default output device: " << Utils::getIMMDeviceProperty(currentDevice, PKEY_Device_FriendlyName) << endl;
//	discoverSessions();
//}
//
//void VolumeManager::setOutputDeviceVolume(float volume) {
//	if (volume < 0 || volume > 1) return;
//	if (currentDeviceVolume) {
//		currentDeviceVolume->SetMasterVolumeLevelScalar(volume, NULL);
//	}
//}
//
//float VolumeManager::getOutputDeviceVolume() {
//	if (currentDeviceVolume) {
//		float level;
//		currentDeviceVolume->GetMasterVolumeLevelScalar(&level);
//		return level;
//	}
//	return -1;
//}
//
//void VolumeManager::setOutputDeviceMute(bool mute) {
//	if (currentDeviceVolume) {
//		currentDeviceVolume->SetMute(mute, NULL);
//	}
//}
//
//bool VolumeManager::getOutputDeviceMute() {
//	if (currentDeviceVolume) {
//		BOOL mute;
//		currentDeviceVolume->GetMute(&mute);
//		return mute ? true : false;
//	}
//	return false;
//}
//
//void VolumeManager::initCurrentOutputDevice() {
//	currentDeviceVolume.Release();
//	currentDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**) &currentDeviceVolume);
//	if (sessionManager) {
//		cout << "UNREGISTER session notification" << endl;
//		sessionManager->UnregisterSessionNotification(sessionNotification);
//	}
//	sessionManager.Release();
//	currentDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**) &sessionManager);
//
//	cout << "REGISTER session notification" << endl;
//	sessionManager->RegisterSessionNotification(sessionNotification);
//
//	CComPtr<IAudioSessionEnumerator> sessionEnumerator;
//	sessionManager->GetSessionEnumerator(&sessionEnumerator);
//	int count;
//	sessionEnumerator->GetCount(&count); // required to start receiving session notifications
//	sessionEnumerator.Release();
//}

//==========================================================
//====================AUDIO SESSIONS========================
//==========================================================

//vector<AudioSession> VolumeManager::discoverSessions() {
//	if (!currentDevice) return vector<AudioSession>();
//	cout << "discovering sessions..." << endl;
//	
//	for (AudioSession& session : audioSessions)
//		session.volume.Release();
//	audioSessions.clear();
//
//	CComPtr<IAudioSessionEnumerator> sessionEnumerator;
//	sessionManager->GetSessionEnumerator(&sessionEnumerator);
//	int count;
//	sessionEnumerator->GetCount(&count);
//	for (int i = 0; i < count; i++) {
//		CComPtr<IAudioSessionControl> sessionControl;
//		CComPtr<IAudioSessionControl2> sessionControl2;
//		sessionEnumerator->GetSession(i, &sessionControl);
//		sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**) &sessionControl2);
//
//		HRESULT isSystem = sessionControl2->IsSystemSoundsSession();
//		DWORD pid;
//		sessionControl2->GetProcessId(&pid);
//		wstring name = Utils::getProcessName(pid);
//
//		AudioSession session;
//		session.isSystemSounds = isSystem == S_OK ? true : false;
//		session.pid = pid;
//		session.filename = isSystem == S_OK ? L"system" : name;
//		sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**) &session.volume);
//		audioSessions.push_back(session);
//
//		sessionControl2.Release();
//		sessionControl.Release();
//	}
//
//	sessionEnumerator.Release();
//	return audioSessions;
//}
//
//void VolumeManager::setSessionVolume(int index, float volume) {
//	if (index < 0 || index >= audioSessions.size()) return;
//	if (volume < 0 || volume > 1) return;
//	audioSessions[index].volume->SetMasterVolume(volume, NULL);
//}
//
//void VolumeManager::setSessionVolume(wstring name, float volume) {
//	setSessionVolume(sessionNameToIndex(name), volume);
//}
//
//float VolumeManager::getSessionVolume(int index) {
//	if (index < 0 || index >= audioSessions.size()) return -1;
//	float level;
//	audioSessions[index].volume->GetMasterVolume(&level);
//	return level;
//}
//
//float VolumeManager::getSessionVolume(wstring name) {
//	return getSessionVolume(sessionNameToIndex(name));
//}
//
//void VolumeManager::setSessionMute(int index, bool mute) {
//	if (index < 0 || index >= audioSessions.size()) return;
//	audioSessions[index].volume->SetMute(mute ? 1 : 0, NULL);
//}
//
//void VolumeManager::setSessionMute(wstring name, bool mute) {
//	return setSessionMute(sessionNameToIndex(name), mute);
//}
//
//bool VolumeManager::getSessionMute(int index) {
//	if (index < 0 || index >= audioSessions.size()) return false;
//	BOOL mute;
//	audioSessions[index].volume->GetMute(&mute);
//	return mute ? true : false;
//}
//
//bool VolumeManager::getSessionMute(wstring name) {
//	return getSessionMute(sessionNameToIndex(name));
//}

void VolumeManager::cleanup() {
	_sessionPool.clear();
	_devicePool.clear();

	_outputDevices.Release();
	_deviceEnumerator.Release();
	//for (AudioSession& session : audioSessions)
	//	session.volume.Release();
	//audioSessions.clear();
	//if (sessionManager && sessionNotification) {
	//	sessionManager->UnregisterSessionNotification(sessionNotification);
	//	delete sessionNotification;
	//	sessionNotification = nullptr;
	//}
	//sessionManager.Release();
	//currentDeviceVolume.Release();
	//currentDevice.Release();
	//outputDevices.Release();
	//deviceEnumerator.Release();
	CoUninitialize();
}

//int VolumeManager::sessionNameToIndex(wstring name) {
//	for (int i = 0; i < audioSessions.size(); i++)
//		if (audioSessions[i].filename == name)
//			return i;
//
//	return -1;
//}