#include "globals.h"
#include "session_interface.h"
#include "utils.h"

#include <iostream>

#include <Windows.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <Functiondiscoverykeys_devpkey.h>

using namespace std;

ISession::~ISession() {
	
}

wstring ISession::getInstanceID() {
	return _instanceID;
}

//////////////////////////////////////////
////// AudioDevice ///////////////////////
//////////////////////////////////////////

AudioDevice::AudioDevice(CComPtr<IMMDevice> immDevice, Controller* controller)
	:_immDevice(immDevice), _controller(controller)
{
	_name = Utils::getIMMDeviceProperty(immDevice, PKEY_Device_FriendlyName);
	_immDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&_endpointVolume);
	_immDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&_sessionManager);
	_sessionManager->GetSessionEnumerator(&_sessionEnumerator);
	notif = new SessionCreatedNotification(controller);
	_sessionManager->RegisterSessionNotification(notif);
	//int dummy;
	//_sessionEnumerator->GetCount(&dummy); // to start receiving notifications
	LPWSTR instanceID;
	if (_immDevice->GetId(&instanceID) != S_OK)
		_instanceID = L"unknown_instance_id";
	else
		_instanceID = instanceID;
	CoTaskMemFree(instanceID);
	DBG_PRINTW(L" > AudioDevice() " << _name << std::endl);
}

AudioDevice::~AudioDevice() {
	DBG_PRINTW(L" > ~AudioDevice() " << _name << endl);
	ISession::~ISession();
	_endpointVolume.Release();
	_sessionEnumerator.Release();
	_sessionManager->UnregisterSessionNotification(notif);
	_sessionManager.Release();
	_immDevice.Release();
	delete notif;
}

void AudioDevice::setVolume(float volume) {
	_endpointVolume->SetMasterVolumeLevelScalar(volume, NULL);
}

float AudioDevice::getVolume() {
	float level = 0;
	_endpointVolume->GetMasterVolumeLevelScalar(&level);
	return level;
}

void AudioDevice::setMute(bool mute) {
	_endpointVolume->SetMute(mute ? TRUE : FALSE, NULL);
}

bool AudioDevice::getMute() {
	BOOL mute;
	_endpointVolume->GetMute(&mute);
	return mute ? true : false;
}

CComPtr<IAudioSessionManager2> AudioDevice::getSessionManager() {
	return _sessionManager;
}

CComPtr<IAudioSessionEnumerator> AudioDevice::getSessionEnumerator() {
	return _sessionEnumerator;
}

//////////////////////////////////////////
////// AudioSession //////////////////////
//////////////////////////////////////////

AudioSession::AudioSession(CComPtr<IAudioSessionControl> sessionControl, Controller* controller)
	: _sessionControl(sessionControl), _controller(controller)
{
	_sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&_simpleVolume);

	CComPtr<IAudioSessionControl2> sessionControl2;
	_sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);

	LPWSTR instanceID;
	if (sessionControl2->GetSessionInstanceIdentifier(&instanceID) != S_OK)
		_instanceID = L"unknown_instance_id";
	else
		_instanceID = instanceID;
	CoTaskMemFree(instanceID);

	bool isSystemSounds = sessionControl2->IsSystemSoundsSession() == S_OK ? true : false;
	if (isSystemSounds) {
		_pid = 0;
		_name = L"system";
	}
	else {
		sessionControl2->GetProcessId(&_pid);
		_name = Utils::getProcessName(_pid);
	}

	_sessionEvents = new AudioSessionEvents(controller, this);
	_sessionControl->RegisterAudioSessionNotification(_sessionEvents);
	_sessionControl->GetState(&_state);
	DBG_PRINTW(L"  - AudioSession(" << _pid << L") " << _name << endl);
	sessionControl2.Release();
}

AudioSession::~AudioSession() {
	DBG_PRINTW(L"  - ~AudioSession() " << _name << endl);
	_sessionControl->UnregisterAudioSessionNotification(_sessionEvents);
	delete _sessionEvents;
	_sessionEvents = nullptr;
	ISession::~ISession();
	_simpleVolume.Release();
	_sessionControl.Release();
}

void AudioSession::setVolume(float volume) {
	_simpleVolume->SetMasterVolume(volume, NULL);
}

float AudioSession::getVolume() {
	float level = 0;
	_simpleVolume->GetMasterVolume(&level);
	return level;
}

void AudioSession::setMute(bool mute) {
	_simpleVolume->SetMute(mute ? TRUE : FALSE, NULL);
}

bool AudioSession::getMute() {
	BOOL mute = FALSE;
	_simpleVolume->GetMute(&mute);
	return mute ? true : false;
}

void AudioSession::setState(AudioSessionState state) {
	_state = state;
	//DBG_PRINTW(L"session state changed: " << state << L", id: " << _instanceID << endl);
}

DWORD AudioSession::getPID() {
	return _pid;
}

CComPtr<IAudioSessionControl> AudioSession::getSessionControl() {
	return _sessionControl;
}

AudioSessionState AudioSession::getState() {
	return _state;
}
