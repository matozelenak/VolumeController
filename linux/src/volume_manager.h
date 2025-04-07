#pragma once
#include "globals.h"
#include "threaded_queue.h"
#include "msg.h"
#include "session.h"

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>
#include <memory>
#include <map>

class VolumeManager {

public:
    VolumeManager(std::shared_ptr<ThreadedQueue<Msg>> msgQueue);
    ~VolumeManager();

    bool init();
    void stop();
    void wait();

    void listSinks(bool lck = true, bool wait = false);
    void listSinkInputs(bool lck = true, bool wait = false);

    void setSinkVolume(int index, float volume, bool lck = true);
    void setSinkMute(int index, bool mute, bool lck = true);
    void setSinkInputVolume(int index, float volume, bool lck = true);
    void setSinkInputMute(int index, bool mute, bool lck = true);
    
    void lock();
    void unlock();
    
    void listSinks_sync();
    void listSinkInputs_sync();
    
    std::map<int, Session> *getSessionPool();
    std::map<int, Session> *getDevicePool();
    
private:
    void _contextCallback(pa_context *c);

    void _listSinksCallback(pa_context *c, const pa_sink_info *i, int eol);
    void _processSink(const pa_sink_info *i);
    void _removeSink(int idx);

    void _listSinkInputsCallback(pa_context *c, const pa_sink_input_info *i, int eol);
    void _processSinkInput(const pa_sink_input_info *i);
    void _removeSinkInput(int idx);

    void _subscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx);
    
    std::shared_ptr<ThreadedQueue<Msg>> _msgQueue;
    pa_threaded_mainloop *_mainloop;
    pa_context *_context;
    bool _isContextConnected;
    std::map<int, Session> _sessionPool;
    std::map<int, Session> _devicePool;

    bool _listSinksInProgress;
    bool _listSinkInputsInProgress;
};