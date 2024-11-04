#pragma once
#include <audiopolicy.h>

class SessionCreatedNotification : public IAudioSessionNotification {
	
public:
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);
	HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl* newSession) override;

private:
	LONG refCount = 1;
};