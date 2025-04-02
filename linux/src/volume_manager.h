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
#include <vector>

class VolumeManager {

public:
    VolumeManager(std::shared_ptr<ThreadedQueue<Msg>> msgQueue);
    ~VolumeManager();

    bool init();
    void stop();
    void wait();

    void listSinks(bool lck = true, bool wait = false);
    void listSinkInputs(bool lck = true, bool wait = false);

    void contextCallback(pa_context *c);
    void listSinksCallback(pa_context *c, const pa_sink_info *i, int eol);
    void processSink(const pa_sink_info *i);
    void removeSink(int idx);
    void listSinkInputsCallback(pa_context *c, const pa_sink_input_info *i, int eol);
    void processSinkInput(const pa_sink_input_info *i);
    void removeSinkInput(int idx);
    void subscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx);

    void lock();
    void unlock();

    void listSinks_sync();
    void listSinkInputs_sync();

    std::vector<Session> *getSessionPool();
    std::vector<Session> *getDevicePool();

private:
    std::shared_ptr<ThreadedQueue<Msg>> _msgQueue;
    pa_threaded_mainloop *_mainloop;
    pa_context *_context;
    bool _isContextConnected;
    std::vector<Session> _sessionPool;
    std::vector<Session> _devicePool;

    bool _listSinksInProgress;
    bool _listSinkInputsInProgress;
};