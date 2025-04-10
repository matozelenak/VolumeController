#include "controller.h"
#include "io.h"
#include "volume_manager.h"
#include "session.h"
#include "utils.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <set>
using namespace std;

Controller::Controller(shared_ptr<IO> io, shared_ptr<VolumeManager> mgr)
    : _io(io), _mgr(mgr) {
    _channels.resize(NUM_CHANNELS, Channel(_mgr));
    _chOther = -1;

    // TODO temporary until config is implemented
    _channelMap = {{"master"}, {"other"}, {"Discord"}, {"Firefox"}, {"javaw"}, {"Vlc"}};
}

Controller::~Controller() {

}

void Controller::remapChannels() {
    _chOther = -1;
    _mgr->listSinks_sync();
    _mgr->listSinkInputs_sync();

    LOG("remapping channels:");
    for (Channel &channel : _channels) channel.clear();

    DevicePool *devicePool = _mgr->getDevicePool();
    SessionPool *sessionPool = _mgr->getSessionPool();
    
    for (int i = 0; i < NUM_CHANNELS; i++) {
        vector<string> &channelApps = _channelMap[i];
        for (string &app : channelApps) {
            if (app == "other") {
                _chOther = i;
            }
            else if (app == "master") {
                Session &dev = (*devicePool)[_mgr->getDefaultSinkIndex()];
                _channels[i].addDevice(dev);
                LOG("  -(ch " << i << ") DEFAULT_SINK " << dev.description);
            }
        }
    }
    LOG("  -chOther is " << _chOther);
    
    for (auto &entry : *devicePool) addDevice(entry.first);
    for (auto &entry : *sessionPool) addSession(entry.first);
    
}

void Controller::defaultSinkChanged() {
    remapChannels();
}

void Controller::addDevice(int index) {
    Session &device = _mgr->getDevicePool()->operator[](index);
    for (int i = 0; i < NUM_CHANNELS; i++) {
        vector<string> &channelApps = _channelMap[i];
        Channel &channel = _channels[i];

        for (string &app : channelApps) {
            if (app == "master" || app == "other") continue;
            if (app == device.description) {
                channel.addDevice(device);
                LOG("  -(ch " << i << ") " << device.description);
                return;
            }
        }
    }
}

void Controller::addSession(int index) {
    Session &session = _mgr->getSessionPool()->operator[](index);
    for (int i = 0; i < NUM_CHANNELS; i++) {
        vector<string> &channelApps = _channelMap[i];
        Channel &channel = _channels[i];

        for (string &app : channelApps) {
            if (app == "master" || app == "other") continue;
            if (app == session.name) {
                channel.addSession(session);
                LOG("  -(ch " << i << ") " << session.name);
                return;
            }
        }
    }

    // add to _chOther
    if (_chOther == -1) return;
    _channels[_chOther].addSession(session);
    LOG("  -(ch " << _chOther << ") " << session.name);
}
