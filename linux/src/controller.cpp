#include "controller.h"
#include "io.h"
#include "volume_manager.h"
#include "session.h"
#include "utils.h"
#include "threaded_queue.h"
#include "msg.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <set>
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

Controller::Controller(shared_ptr<IO> io, shared_ptr<VolumeManager> mgr, Config &cfg, std::shared_ptr<ThreadedQueue<Msg>> msgQueue)
    : _io(io), _mgr(mgr), _cfg(&cfg), _msgQueue(msgQueue) {
    _channels.resize(NUM_CHANNELS, Channel(_mgr));
    _chOther = -1;

    _channelMap = cfg.channels;
    // _channelMap = {{"master"}, {"other"}, {"Discord"}, {"Firefox"}, {"javaw"}, {"Vlc"}};
}

Controller::~Controller() {

}

void Controller::remapChannels() {
    _channelActive.clear();
    _channelActive.resize(NUM_CHANNELS, false);
    _chOther = -1;
    _mgr->listSinks_sync();
    _mgr->listSinkInputs_sync();

    LOG("remapping channels:");
    for (Channel &channel : _channels) channel.clear();

    DevicePool *devicePool = _mgr->getDevicePool();
    SessionPool *sessionPool = _mgr->getSessionPool();
    
    for (int i = 0; i < _channelMap.size(); i++) {
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
    
    for (auto &entry : *devicePool) addDevice(entry.first, false);
    for (auto &entry : *sessionPool) addSession(entry.first, false);

    for (int i = 0; i < NUM_CHANNELS; i++)
        _channelActive[i] = _channels[i].isActive();
    
    updateController();
}

void Controller::defaultSinkChanged() {
    remapChannels();
}

void Controller::addDevice(int index, bool update) {
    if (update) {
        json data;
        makeCompleteJSON(data);
        _io->sendPipe(data.dump().c_str());
    }
    Session &device = _mgr->getDevicePool()->operator[](index);
    for (int i = 0; i < _channelMap.size(); i++) {
        vector<string> &channelApps = _channelMap[i];
        Channel &channel = _channels[i];

        for (string &app : channelApps) {
            if (app == "master" || app == "other") continue;
            if (app == device.description) {
                bool ret = channel.addDevice(device);
                LOG("  -(ch " << i << ") " << device.description);
                if (update && ret != _channelActive[i]) {
                    _channelActive[i] = ret;
                    updateController();
                }
                return;
            }
        }
    }
}

void Controller::addSession(int index, bool update) {
    if (update) {
        json data;
        makeCompleteJSON(data);
        _io->sendPipe(data.dump().c_str());
    }
    Session &session = _mgr->getSessionPool()->operator[](index);
    for (int i = 0; i < _channelMap.size(); i++) {
        vector<string> &channelApps = _channelMap[i];
        Channel &channel = _channels[i];

        for (string &app : channelApps) {
            if (app == "master" || app == "other") continue;
            if (app == session.name) {
                bool ret = channel.addSession(session);
                LOG("  -(ch " << i << ") " << session.name);
                if (update && ret != _channelActive[i]) {
                    _channelActive[i] = ret;
                    updateController();
                }
                return;
            }
        }
    }

    // add to _chOther
    if (_chOther == -1) return;
    _channels[_chOther].addSession(session);
    LOG("  -(ch " << _chOther << ") " << session.name);
}

void Controller::removeDevice(int index) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        Channel &channel = _channels[i];
        bool ret = channel.removeDevice(index);
        if (ret != _channelActive[i]) {
            _channelActive[i] = ret;
            updateController();
        }
    }
}

void Controller::removeSession(int index) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        Channel &channel = _channels[i];
        bool ret = channel.removeSession(index);
        if (ret != _channelActive[i]) {
            _channelActive[i] = ret;
            updateController();
        }
    }
}

void Controller::parseData(string &data) {
    if (data == "READY") {
        updateController();
        return;
    }
    CMDWHAT what = Utils::getWhat(data);
    CMDTYPE type = Utils::getType(data);
    
    if (what == CMDWHAT::COMMAND) {
        switch (type)
        {
        case CMDTYPE::MUTE:
            if (data[2] != ':') { // TODO temporary
                char cmd;
                int ch;
                int val;
                if (!Utils::parseCmd1Value(data, cmd, ch, val)) break;
                _channels[ch].setMute(val);
                // LOG("cmd: " << cmd << " ch " << ch << " val " << val);
            }
            break;
        case CMDTYPE::VOLUME:
            if (data[2] != ':') { // TODO temporary
                char cmd;
                int ch;
                int val;
                if (!Utils::parseCmd1Value(data, cmd, ch, val)) break;
                _channels[ch].setVolume(val/100.0);
                // LOG("cmd: " << cmd << " ch " << ch << " val " << val);
            }
            else {
                char cmd;
                int vals[NUM_CHANNELS];
                if (!Utils::parseCmdAllValues(data, cmd, vals)) break;
                for (int i = 0; i < NUM_CHANNELS; i++) _channels[i].setVolume(vals[i]/100.0);
            }
            break;
        
        default:
            break;
        }
    } else if (what == CMDWHAT::REQUEST) {
        switch (type)
        {
        case CMDTYPE::ACTIVE_CH:
            sendActiveData();
            break;
        case CMDTYPE::MUTE:
            sendMuteData();
            break;
        default:
            break;
        }
    }
}

void Controller::updateController() {
    sendActiveData();
    sendMuteData();
    requestVolumeData();
}

void Controller::sendActiveData() {
    int activeData[NUM_CHANNELS];
    for (int i = 0; i < NUM_CHANNELS; i++)
        activeData[i] = _channels[i].isActive() ? 1 : 0;
    _io->sendSerial(Utils::makeCmdAllValues(CMDTYPE::ACTIVE_CH, activeData).c_str());
}

void Controller::sendMuteData() {
    int muteData[NUM_CHANNELS];
    for (int i = 0; i < NUM_CHANNELS; i++)
        muteData[i] = _channels[i].getMute() ? 1 : 0;
    _io->sendSerial(Utils::makeCmdAllValues(CMDTYPE::MUTE, muteData).c_str());
}

void Controller::requestVolumeData() {
    _io->sendSerial(Utils::makeRequest(CMDTYPE::VOLUME).c_str());
}

void Controller::configChanged(Config &cfg) {
    _channelMap = cfg.channels;
    remapChannels();
}

void Controller::handlePipeData(string &str) {
    json data;
    try {
        data = json::parse(str);
    } catch (json::parse_error &e) {
        LOG(e.what());
        return;
    }
    json response = json();
    if (data.contains("request")) {
        for (string type : data["request"]) {
            if (type == "status") {
                makeStatusJSON(response);
            }
            else if (type == "config") {
                response["configFile"] = Utils::storeConfigToJSON(*_cfg);
            }
            else if (type == "devicePool") {
                DevicePool *devicePool = _mgr->getDevicePool();
                response["devicePool"] = json::array();
                for (auto &entry : *devicePool) {
                    response["devicePool"].push_back(entry.second.description);
                }
            }
            else if (type == "sessionPool") {
                SessionPool *sessionPool = _mgr->getSessionPool();
                response["sessionPool"] = json::array();
                for (auto &entry : *sessionPool) {
                    response["sessionPool"].push_back(entry.second.name);
                }
            }
        }
        _io->sendPipe(response.dump().c_str());
    }
    
    if (data.contains("configFile")) {
        json &configFile = data["configFile"];
        Utils::parseConfigFromJSON(configFile, *_cfg);
        Utils::writeConfig(*_cfg);
        _msgQueue->pushAndSignal(Msg{MsgType::CONFIG_CHANGED});
    }
}

void Controller::makeStatusJSON(nlohmann::json &response) {
    json status = json();
    status["deviceConnected"] = _io->isSerialConnected();
    status["comPorts"] = json::array();
    for (string &portName : Utils::getSerialPorts()) {
        status["comPorts"].push_back(portName);
    }
    response["status"] = status;
}

void Controller::makeCompleteJSON(nlohmann::json &data) {
    makeStatusJSON(data["status"]);
    data["configFile"] = Utils::storeConfigToJSON(*_cfg);
    DevicePool *devicePool = _mgr->getDevicePool();
    data["devicePool"] = json::array();
    for (auto &entry : *devicePool) {
        data["devicePool"].push_back(entry.second.description);
    }
    SessionPool *sessionPool = _mgr->getSessionPool();
    data["sessionPool"] = json::array();
    for (auto &entry : *sessionPool) {
        data["sessionPool"].push_back(entry.second.name);
    }
}