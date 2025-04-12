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
    void addDevice(int index, bool update = true);
    void addSession(int index, bool update = true);
    void removeDevice(int index);
    void removeSession(int index);

    void updateController();
    void sendActiveData();
    void sendMuteData();
    void requestVolumeData();

private:
    std::shared_ptr<IO> _io;
    std::shared_ptr<VolumeManager> _mgr;
    std::vector<Channel> _channels;
    std::vector<bool> _channelActive;

    int _chOther;
    std::vector<std::vector<std::string>> _channelMap;
};