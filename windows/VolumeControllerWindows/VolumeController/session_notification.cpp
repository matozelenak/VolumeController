#include "session_notification.h"
#include <iostream>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <atlbase.h>
#include <functiondiscoverykeys_devpkey.h>

using namespace std;

//////////////////////////////////////////
////// SessionCreatedNotification /////////
//////////////////////////////////////////

SessionCreatedNotification::SessionCreatedNotification(Controller* controller) {
	this->controller = controller;
}

ULONG STDMETHODCALLTYPE SessionCreatedNotification::AddRef() {
	return InterlockedIncrement(&refCount);
}

ULONG STDMETHODCALLTYPE SessionCreatedNotification::Release() {
	ULONG ulRef = InterlockedDecrement(&refCount);
	if (0 == ulRef) {
		delete this;
	}
	return ulRef;
}

HRESULT STDMETHODCALLTYPE SessionCreatedNotification::QueryInterface(REFIID riid, VOID** ppvInterface) {
	if (IID_IUnknown == riid) {
		*ppvInterface = (IUnknown*)this;
	}
	else if (__uuidof(IAudioSessionNotification) == riid) {
		*ppvInterface = (IAudioSessionNotification*)this;
	}
	else {
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SessionCreatedNotification::OnSessionCreated(IAudioSessionControl* newSession) {
	CComPtr<IAudioSessionControl2> sessionControl2;
	newSession->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
	DWORD pid;
	sessionControl2->GetProcessId(&pid);
	sessionControl2.Release();

	DBG_PRINT("session created, pid: " << pid << endl);
	controller->createSession(newSession);
	return S_OK;
}

//////////////////////////////////////////
////// AudioDeviceNotification ///////////
//////////////////////////////////////////

AudioDeviceNotification::AudioDeviceNotification(Controller* controller)
    :controller(controller) {

}

ULONG STDMETHODCALLTYPE AudioDeviceNotification::AddRef() {
	return InterlockedIncrement(&refCount);
}

ULONG STDMETHODCALLTYPE AudioDeviceNotification::Release() {
	ULONG ulRef = InterlockedDecrement(&refCount);
	if (0 == ulRef) {
		delete this;
	}
	return ulRef;
}

HRESULT STDMETHODCALLTYPE AudioDeviceNotification::QueryInterface(REFIID riid, VOID** ppvInterface) {
	if (IID_IUnknown == riid) {
		*ppvInterface = (IUnknown*)this;
	}
	else if (__uuidof(IMMNotificationClient) == riid) {
		*ppvInterface = (IMMNotificationClient*)this;
	}
	else {
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioDeviceNotification::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
    controller->defaultDeviceChanged(flow, role);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioDeviceNotification::OnDeviceAdded(LPCWSTR pwstrDeviceId) {
    controller->deviceAddedOrRemoved();
    return S_OK;
};

HRESULT STDMETHODCALLTYPE AudioDeviceNotification::OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
    controller->deviceAddedOrRemoved();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioDeviceNotification::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioDeviceNotification::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) {
    return S_OK;
}

//////////////////////////////////////////
////////// AudioSessionEvents ////////////
//////////////////////////////////////////

AudioSessionEvents::AudioSessionEvents(Controller* controller, AudioSession* session)
    :_controller(controller), _session(session) {

}

ULONG AudioSessionEvents::AddRef() {
    return InterlockedIncrement(&refCount);
}

ULONG AudioSessionEvents::Release() {
    ULONG ulRef = InterlockedDecrement(&refCount);
    if (ulRef == 0) {
        delete this;
    }
    return ulRef;
}

HRESULT AudioSessionEvents::QueryInterface(REFIID riid, void** ppvObject) {
    if (riid == IID_IUnknown || riid == __uuidof(IAudioSessionEvents)) {
        *ppvObject = static_cast<IAudioSessionEvents*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

HRESULT AudioSessionEvents::OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext) {
    return S_OK;
}

HRESULT AudioSessionEvents::OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext) {
    return S_OK;
}

HRESULT AudioSessionEvents::OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext) {
    return S_OK;
}

HRESULT AudioSessionEvents::OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolume[], DWORD ChangedChannel, LPCGUID EventContext) {
    return S_OK;
}

HRESULT AudioSessionEvents::OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext) {
    return S_OK;
}

HRESULT AudioSessionEvents::OnStateChanged(AudioSessionState NewState) {
    _session->setState(NewState);
    if (NewState == AudioSessionStateExpired)
        PostMessage(_controller->getHWND(), MSG_SESSION_DESTROYED, NULL, NULL);
    return S_OK;
}

HRESULT AudioSessionEvents::OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason) {
    //DBG_PRINTW(L"session destroyed, instanceID: " << _session->getInstanceID() << endl);
    return S_OK;
}