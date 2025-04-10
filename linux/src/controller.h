#pragma once
#include "globals.h"
#include "io.h"
#include "volume_manager.h"
#include "session.h"
#include "utils.h"
#include "channel.h"

#include <vector>
#include <string>
#include <map>
#include <memory>

class Controller {
public:
    Controller(std::shared_ptr<IO> io, std::shared_ptr<VolumeManager> mgr);
    ~Controller();

    void remapChannels();
    void defaultSinkChanged();
    void addDevice(int index);
    void addSession(int index);

private:
    std::shared_ptr<IO> _io;
    std::shared_ptr<VolumeManager> _mgr;
    std::vector<Channel> _channels;

    int _chOther;
    std::vector<std::vector<std::string>> _channelMap;
};