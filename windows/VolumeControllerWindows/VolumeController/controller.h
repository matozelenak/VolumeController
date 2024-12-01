#pragma once

#include "volume_manager.h"
#include "io.h"
#include "structs.h"

class Controller {

public:
	Controller(VolumeManager* mgr, Config* config);
	~Controller();

	void mapChannels();

	void adjustVolume(int ch, int val);
	void adjustMute(int ch, bool mute);

	std::string makeVolumeRequest();
	std::string makeMuteRequest();
	std::string makeVolumeCmd();
	std::string makeMuteCmd();
	std::string makeActiveDataCmd();

	void update(IO* io);

	std::vector<AudioSession> getSessions();
private:
	VolumeManager* mgr;
	Config* config;
	std::vector<Channel> channels;
	std::vector<AudioSession> sessions;
};