#pragma once
#include "globals.h"
#include "controller.h"
#include <audiopolicy.h>
#include <mmdeviceapi.h>

//////////////////////////////////////////
////// SessionCreatedNotification /////////
//////////////////////////////////////////

class SessionCreatedNotification : public IAudioSessionNotification {
	
public:
	SessionCreatedNotification(Controller* controller);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);
	HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl* newSession) override;

private:
	LONG refCount = 1;
	Controller* controller;
};

//////////////////////////////////////////
////// AudioDeviceNotification ///////////
//////////////////////////////////////////


class AudioDeviceNotification : public IMMNotificationClient {

public:
    AudioDeviceNotification(Controller* controller);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);

private:
    LONG refCount = 1;
    Controller* controller;

};

//////////////////////////////////////////
////////// AudioSessionEvents ////////////
//////////////////////////////////////////

class AudioSessionEvents : public IAudioSessionEvents {
public:
    AudioSessionEvents(Controller* controller, AudioSession* session);

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);

    HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolume[], DWORD ChangedChannel, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState NewState);
    HRESULT STDMETHODCALLTYPE OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason);

private:
    LONG refCount = 1;
    Controller* _controller;
    AudioSession* _session;
};