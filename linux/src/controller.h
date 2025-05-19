#pragma once
#include "globals.h"
#include "io.h"
#include "volume_manager.h"
#include "session.h"
#include "utils.h"
#include "channel.h"
#include "threaded_queue.h"
#include "msg.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "nlohmann/json.hpp"

class Controller {
public:
    Controller(std::shared_ptr<IO> io, std::shared_ptr<VolumeManager> mgr, Config &cfg, std::shared_ptr<ThreadedQueue<Msg>> msgQueue);
    ~Controller();

    void remapChannels();
    void defaultSinkChanged();
    void addDevice(int index, bool update = true);
    void addSession(int index, bool update = true);
    void removeDevice(int index);
    void removeSession(int index);

    void parseData(std::string &data);
    void updateController();
    void sendActiveData();
    void sendMuteData();
    void requestVolumeData();

    void configChanged(Config &cfg);

    void handlePipeData(std::string &str);
    void makeStatusJSON(nlohmann::json &response);
    void makeCompleteJSON(nlohmann::json &data);

private:
    std::shared_ptr<IO> _io;
    std::shared_ptr<VolumeManager> _mgr;
    std::shared_ptr<ThreadedQueue<Msg>> _msgQueue;
    std::vector<Channel> _channels;
    std::vector<bool> _channelActive;
    Config *_cfg;

    int _chOther;
    std::vector<std::vector<std::string>> _channelMap;
};