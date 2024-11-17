#pragma once

#include "volume_manager.h"
#include "io.h"
#include "structs.h"

class Controller {

public:
	Controller(VolumeManager* mgr);
	~Controller();

	void mapChannels();
	void readConfig();

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
	std::vector<Channel> channels;
	std::vector<AudioSession> sessions;

	std::vector<std::vector<std::wstring>> configTmp;
};