#pragma once
#include "globals.h"
#include "session_interface.h"
#include <vector>

class Channel {

public:
	Channel(int id);
	void setVolume(int volume);
	void setMute(bool mute);
	void addSession(PISession session);
	void removeSession(std::wstring instanceID);
	void removeDestroyedSessions();

	int getID();
	bool isMuted();
	bool isActive();
	std::vector<PISession>& getSessions();

private:
	int id = 0;
	bool active = false;
	float volume = 0;
	bool muted = false;
	std::vector<PISession> sessions;
};