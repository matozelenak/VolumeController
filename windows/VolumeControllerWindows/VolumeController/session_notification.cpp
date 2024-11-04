#include "session_notification.h"
#include <audiopolicy.h>
#include <atlbase.h>
#include <iostream>
using namespace std;

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

	cout << "session created, pid: " << pid << endl;
	return S_OK;
}