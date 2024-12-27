#include "globals.h"
#include "volume_manager.h"
#include "session_interface.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <vector>

#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <Functiondiscoverykeys_devpkey.h>

using namespace std;

VolumeManager::VolumeManager(Controller* controller) {
	this->controller = controller;
}

VolumeManager::~VolumeManager() {
	cleanup();
}

bool VolumeManager::initialize(HWND hWnd) {
	if (CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&_deviceEnumerator)) != S_OK) {
		return false;
	}

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

void VolumeManager::cleanup() {
	_sessionPool.clear();
	_devicePool.clear();

	_outputDevices.Release();
	_deviceEnumerator.Release();
}
