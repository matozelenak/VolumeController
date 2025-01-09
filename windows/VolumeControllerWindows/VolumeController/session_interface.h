#pragma once
#include "globals.h"
#include "session_notification.h"
#include <string>
#include <memory>

#include <Windows.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>


class ISession {

public:
	virtual ~ISession();

	virtual void setVolume(float volume) = 0;
	virtual float getVolume() = 0;
	virtual void setMute(bool mute) = 0;
	virtual bool getMute() = 0;
	virtual std::wstring getName() { return _name; }
	std::wstring getInstanceID();

protected:
	std::wstring _name;
	std::wstring _instanceID;
};

//////////////////////////////////////////
////// AudioDevice ///////////////////////
//////////////////////////////////////////

class AudioDevice : public ISession {

public:
	AudioDevice(CComPtr<IMMDevice> immDevice, Controller* controller);
	~AudioDevice();

	void setVolume(float volume);
	float getVolume();
	void setMute(bool mute);
	bool getMute();

	CComPtr<IAudioSessionManager2> getSessionManager();
	CComPtr<IAudioSessionEnumerator> getSessionEnumerator();

private:
	CComPtr<IMMDevice> _immDevice;
	CComPtr<IAudioEndpointVolume> _endpointVolume;
	CComPtr<IAudioSessionManager2> _sessionManager;
	CComPtr<IAudioSessionEnumerator> _sessionEnumerator;
	Controller* _controller;
	SessionCreatedNotification* notif;
};

//////////////////////////////////////////
////// AudioSession //////////////////////
//////////////////////////////////////////

class AudioSession : public ISession {

public:
	AudioSession(CComPtr<IAudioSessionControl> sessionControl, Controller* controller);
	~AudioSession();

	void setVolume(float volume);
	float getVolume();
	void setMute(bool mute);
	bool getMute();

	void setState(AudioSessionState state);

	DWORD getPID();
	CComPtr<IAudioSessionControl> getSessionControl();
	AudioSessionState getState();

private:
	DWORD _pid;
	CComPtr<IAudioSessionControl> _sessionControl;
	CComPtr<ISimpleAudioVolume> _simpleVolume;
	Controller* _controller;
	AudioSessionEvents* _sessionEvents;
	AudioSessionState _state;
};


