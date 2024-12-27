#include "globals.h"
#include "channel.h"
#include "session_interface.h"
#include <vector>
#include <string>

using namespace std;

Channel::Channel(int id)
	:id(id) {

}

void Channel::setVolume(int volume) {
	if (volume < 0 || volume > 100) return;
	this->volume = volume / 100.0f;
	for (PISession& session : sessions) {
		session->setVolume(this->volume);
	}
}

void Channel::setMute(bool mute) {
	this->muted = mute;
	for (PISession& session : sessions) {
		session->setMute(mute);
	}
}

void Channel::addSession(PISession session) {
	//if (session == nullptr) return;
	sessions.push_back(session);
	if (active) {
		session->setVolume(this->volume);
		session->setMute(this->muted);
	}
	else {
		volume = session->getVolume();
		muted = session->getMute();
		active = true;
	}
}

void Channel::removeSession(wstring instanceID) {
	for (int i = 0; i < sessions.size(); i++) {
		PISession& session = sessions[i];
		if (session->getInstanceID() == instanceID) {
			sessions.erase(sessions.begin() + i);
			break;
		}
	}
}

void Channel::removeDestroyedSessions() {
	for (int i = 0; i < sessions.size(); i++) {
		if (PAudioSession audioSession = dynamic_pointer_cast<AudioSession>(sessions[i])) {
			if (audioSession->getState() == AudioSessionStateExpired) {
				sessions.erase(sessions.begin() + i);
				i--;
			}
		}
	}
}

int Channel::getID() {
	return id;
}

bool Channel::isMuted() {
	return muted;
}

bool Channel::isActive() {
	return active;
}

vector<PISession>& Channel::getSessions() {
	return sessions;
}