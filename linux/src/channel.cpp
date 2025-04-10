#include "channel.h"
#include "volume_manager.h"

#include <memory>
using namespace std;

Channel::Channel(shared_ptr<VolumeManager> mgr)
    : _mgr(mgr) {
    _active = false;
    _volume = 0;
    _mute = false;
}

Channel::~Channel() {

}

bool Channel::addDevice(Session &device) {
    _devices[device.index] = device;
    _mgr->setSinkVolume(device.index, _volume);
    _mgr->setSinkMute(device.index, _mute);
    if (!_active) _active = true;
    return _active;
}

bool Channel::addSession(Session &session) {
    _sessions[session.index] = session;
    _mgr->setSinkInputVolume(session.index, _volume);
    _mgr->setSinkInputMute(session.index, _mute);
    if (!_active) _active = true;
    return _active;
}

bool Channel::removeDevice(int index) {
    _devices.erase(index);
    if (_devices.size() + _sessions.size() == 0) _active = false;
    return _active;
}

bool Channel::removeSession(int index) {
    _sessions.erase(index);
    if (_devices.size() + _sessions.size() == 0) _active = false;
    return _active;
}

void Channel::clear() {
    _active = false;
    _devices.clear();
    _sessions.clear();
}

bool Channel::clearSinks() {
    _devices.clear();
    if (_sessions.empty()) _active = false;
    return _active;
}

bool Channel::clearSinkInputs() {
    _sessions.clear();
    if (_devices.empty()) _active = false;
    return _active;
}

void Channel::setVolume(float volume) {
    _volume = volume;
    for (auto &entry : _devices) 
        _mgr->setSinkVolume(entry.first, _volume);
    for (auto &entry : _sessions) 
        _mgr->setSinkInputVolume(entry.first, _volume);
}

float Channel::getVolume() {
    return _volume;
}

void Channel::setMute(bool mute) {
    _mute = mute;
    for (auto &entry : _devices) 
        _mgr->setSinkMute(entry.first, _mute);
    for (auto &entry : _sessions) 
        _mgr->setSinkInputMute(entry.first, _mute);
}

bool Channel::getMute() {
    return _mute;
}