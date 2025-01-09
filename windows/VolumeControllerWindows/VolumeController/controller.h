#pragma once

#include "volume_manager.h"
#include "io.h"
#include "structs.h"
#include "channel.h"
#include <audiopolicy.h>

class Controller {

public:
	Controller(HWND hWnd, Config* config, IO* io);
	~Controller();

	void rescanAndRemap();
	void mapChannels();
	void addSession(PISession session);
	void createSession(IAudioSessionControl* newSession);
	void sessionDestroyed();
	void defaultDeviceChanged(EDataFlow flow, ERole role);
	void deviceAddedOrRemoved();

	void adjustVolume(int ch, int val);
	void adjustMute(int ch, bool mute);

	std::string makeVolumeRequest();
	std::string makeMuteRequest();
	std::string makeVolumeCmd();
	std::string makeMuteCmd();
	std::string makeActiveDataCmd();

	void update();

	VolumeManager* getVolumeManager();
	std::vector<Channel>& getChannels();
	HWND getHWND();

private:
	VolumeManager* mgr;
	Config* config;
	IO* io;
	std::vector<Channel> channels;
	std::vector<PAudioSession>* sessionPool;
	std::vector<PAudioDevice>* devicePool;
	int chVolOther = -1;
	HWND hWnd;
	AudioDeviceNotification* notif;
};