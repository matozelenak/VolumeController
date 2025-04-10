#pragma once
#include "globals.h"
#include "session.h"
#include "volume_manager.h"

#include <map>
#include <memory>

class Channel {
public:
    Channel(std::shared_ptr<VolumeManager> mgr);
    ~Channel();

    bool addDevice(Session &device);
    bool addSession(Session &session);
    bool removeDevice(int index);
    bool removeSession(int index);
    void clear();
    bool clearSinks();
    bool clearSinkInputs();

    void setVolume(float volume);
    float getVolume();
    void setMute(bool mute);
    bool getMute();

private:
    std::shared_ptr<VolumeManager> _mgr;
    std::map<int, Session> _devices;
    std::map<int, Session> _sessions;
    bool _active;
    float _volume;
    bool _mute;
};