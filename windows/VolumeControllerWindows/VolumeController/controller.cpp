#include "controller.h"
#include "volume_manager.h"
#include "io.h"
#include "structs.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

Controller::Controller(VolumeManager* mgr) {
	this->mgr = mgr;

	configTmp.push_back(vector<wstring>());
	configTmp.push_back(vector<wstring>());
	configTmp.push_back(vector<wstring>());
	configTmp[0].push_back(L"master");
	configTmp[1].push_back(L"system");
	configTmp[2].push_back(L"other");
}

Controller::~Controller() {

}

void Controller::mapChannels() {
	// convert 0: (master) 1: (system, other), 3: (Discord.exe, Teams.exe)
	// to 0: (master) 1: (system, ...), 3:
	channels.clear();
	for (int i = 0; i < 6; i++) {
		channels.push_back(Channel{i});
	}
	sessions.clear();
	sessions = mgr->discoverSessions();

	// map volumes
	vector<bool> chVolUsed(sessions.size());
	int chVolOther = -1;
	// iterate channels
	for (int i = 0; i < configTmp.size(); i++) {
		// iterate applications for that channel
		for (int j = 0; j < configTmp[i].size(); j++) {
			if (configTmp[i][j] != L"other") {

				channels[i].sessions.push_back(configTmp[i][j]);

				// find which one is it
				for (int k = 0; k < sessions.size(); k++) {
					if (sessions[k].filename == configTmp[i][j]) {
						chVolUsed[k] = true;
						break;
					}
				}
			}
			else {
				// this channel will control all other unassigned applications
				chVolOther = i;
			}
		}
	}
	// map all other unassigned applications
	if (chVolOther != -1) {
		for (int i = 0; i < sessions.size(); i++) {
			if (!chVolUsed[i])
				channels[chVolOther].sessions.push_back(sessions[i].filename);
		}
	}
}

void Controller::readConfig() {
	// TODO
}

void Controller::adjustVolume(int ch, int value) {
	for (wstring name : channels[ch].sessions) {
		if (name == L"master")
			mgr->setOutputDeviceVolume(value / 100.0);
		else
			mgr->setSessionVolume(name, value / 100.0);
	}
}

void Controller::adjustMute(int ch, bool mute) {
	for (wstring name : channels[ch].sessions) {
		if (name == L"master") 
			mgr->setOutputDeviceMute(mute);
		else
			mgr->setSessionMute(name, mute);
	}
}

string Controller::makeVolumeRequest() {
	return "?V\n";
}

string Controller::makeMuteRequest() {
	return "?M\n";
}

string Controller::makeVolumeCmd() {
	return "";
}

string Controller::makeMuteCmd() {
	vector<bool> chMute(6);
	for (Channel& channel : channels) {
		for (wstring session : channel.sessions) {
			if (session == L"master") {
				chMute[channel.id] = mgr->getOutputDeviceMute();
				break;
			}
			else {
				for (AudioSession& aSession : sessions) {
					if (aSession.filename == session) {
						chMute[channel.id] = mgr->getSessionMute(session);
						goto end_iterateChannelSessions;
					}
				}
			}
		}

	end_iterateChannelSessions:
		;
	}

	stringstream ss;
	ss << "!M:";
	for (bool b : chMute)
		ss << (b ? "1|" : "0|");
	ss << "\n";

	return ss.str();
}

string Controller::makeActiveDataCmd() {
	vector<bool> chActive(6);
	for (Channel& channel : channels) {
		for (wstring session : channel.sessions) {
			if (session == L"master") {
				chActive[channel.id] = true;
				break;
			}
			else {
				for (AudioSession& aSession : sessions) {
					if (aSession.filename == session) {
						chActive[channel.id] = true;
						goto end_iterateChannelSessions;
					}
				}
			}
		}

	end_iterateChannelSessions:
		;
	}

	stringstream ss;
	ss << "!A:";
	for (bool b : chActive)
		ss << (b ? "1|" : "0|");
	ss << "\n";

	return ss.str();
}

void Controller::update(IO* io) {
	io->send(makeActiveDataCmd());
	io->send(makeMuteCmd());
	io->send(makeVolumeRequest());
	io->send(makeMuteRequest());
}

vector<AudioSession> Controller::getSessions() {
	return sessions;
}
