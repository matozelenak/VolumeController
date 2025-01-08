#include "controller.h"
#include "volume_manager.h"
#include "io.h"
#include "structs.h"
#include "channel.h"
#include "utils.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

Controller::Controller(HWND hWnd, Config* config, IO* io) {
	this->hWnd = hWnd;
	this->config = config;
	this->io = io;
	mgr = new VolumeManager(this);
	if (!mgr->initialize(hWnd)) {
		DBG_PRINT("Error: VolumeManager failed to initialize");
	}
	this->sessionPool = mgr->getSessionPoolAddr();
	this->devicePool = mgr->getDevicePoolAddr();

	notif = new AudioDeviceNotification(this);
	mgr->getDeviceEnumerator()->RegisterEndpointNotificationCallback(notif);
}

Controller::~Controller() {
	mgr->getDeviceEnumerator()->UnregisterEndpointNotificationCallback(notif);
	delete notif;
	delete mgr;
}

void Controller::rescanAndRemap() {
	mgr->scanDevicesAndSessions();
	mapChannels();
}

void Controller::mapChannels() {
	// convert 0: (master) 1: (system, other), 3: (Discord.exe, Teams.exe)
	// to 0: (master) 1: (system, ...), 3:
	channels.clear();
	for (int i = 0; i < 6; i++) {
		channels.push_back(Channel(i));
	}

	// map volumes
	vector<bool> chVolUsed(sessionPool->size());
	chVolOther = -1;
	// iterate channels
	for (int i = 0; i < config->channels.size(); i++) {
		// iterate applications for that channel
		for (int j = 0; j < config->channels[i].size(); j++) {
			wstring sessionName = config->channels[i][j];
			if (sessionName == L"master") {
				PAudioDevice oDevice = mgr->getDefaultOutputDevice();
				if (oDevice)
					channels[i].addSession(oDevice);
			}
			else if (sessionName != L"other") {
				vector<PISession> sessions = mgr->getSession(sessionName);
				for (PISession& session : sessions)
					channels[i].addSession(session);

				// find which one is it
				for (int k = 0; k < sessionPool->size(); k++) {
					if (sessionPool->at(k)->getName() == sessionName) {
						chVolUsed[k] = true;
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
		for (int i = 0; i < sessionPool->size(); i++) {
			if (!chVolUsed[i])
				channels[chVolOther].addSession(sessionPool->at(i));
		}
	}
}

void Controller::addSession(PISession session) {
	wstring name = session->getName();
	for (int i = 0; i < config->channels.size(); i++) {
		vector<wstring> channel = config->channels[i];
		for (int j = 0; j < channel.size(); j++) {
			if (channel[j] == name) {
				channels[i].addSession(session);
				DBG_PRINTW(L"added " << name << " to channel " << i << endl);
				update();
				return;
			}
		}
	}

	// not found, add to the channel which has 'other'
	if (chVolOther != -1) {
		channels[chVolOther].addSession(session);
		DBG_PRINTW(L"added " << name << " to volOther (" << chVolOther << L")" << endl);
		update();
	}
}

void Controller::createSession(IAudioSessionControl* newSession) {
	for (PAudioSession& session : *sessionPool) {
		CComPtr<IAudioSessionControl> control = session->getSessionControl();
		if (control == newSession) {
			DBG_PRINTW(L"this session already exists: " << session->getName() << endl);
			return;
		}
	}
	PAudioSession session = make_shared<AudioSession>(newSession, this);
	sessionPool->push_back(session);
	addSession(session);
}

void Controller::sessionDestroyed() {
	for (Channel& channel : channels) {
		channel.removeDestroyedSessions();
	}

	for (int i = 0; i < sessionPool->size(); i++) {
		PAudioSession& session = sessionPool->at(i);
		if (session->getState() == AudioSessionStateExpired) {
			sessionPool->erase(sessionPool->begin() + i);
			i--;
		}
	}

	update();
}

void Controller::defaultDeviceChanged(EDataFlow flow, ERole role) {
	if (flow == eRender && role == eConsole) {
		DBG_PRINT("default output device was changed" << endl);
		rescanAndRemap();
		update();
	}
}

void Controller::deviceAddedOrRemoved() {
	//cout << "device added or removed" << endl;
}

void Controller::adjustVolume(int ch, int value) {
	if (ch < 0 || ch > 5) return;
	channels[ch].setVolume(value);
}

void Controller::adjustMute(int ch, bool mute) {
	if (ch < 0 || ch > 5) return;
	channels[ch].setMute(mute);
}

string Controller::makeVolumeRequest() {
	return "?V\n";
}

string Controller::makeMuteRequest() {
	return "?M\n";
}

string Controller::makeVolumeCmd() {
	return ""; // TODO
}

string Controller::makeMuteCmd() {
	vector<int> chMute(6);
	for (Channel& channel : channels) {
		chMute[channel.getID()] = channel.isMuted();
	}

	return Utils::makeCmdAllVals('M', chMute);
}

string Controller::makeActiveDataCmd() {
	vector<int> chActive(6);
	for (Channel& channel : channels) {
		chActive[channel.getID()] = channel.isActive();
	}

	return Utils::makeCmdAllVals('A', chActive);
}

void Controller::update() {
	io->send(makeActiveDataCmd());
	io->send(makeMuteCmd());
	io->send(makeVolumeRequest());
	io->send(makeMuteRequest());
}

VolumeManager* Controller::getVolumeManager() {
	return mgr;
}

vector<Channel>& Controller::getChannels() {
	return channels;
}

HWND Controller::getHWND() {
	return hWnd;
}
