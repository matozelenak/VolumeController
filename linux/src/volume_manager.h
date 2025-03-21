#pragma once
#include "globals.h"
#include "threaded_queue.h"
#include "msg.h"

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>
#include <memory>

class VolumeManager {

public:
    VolumeManager(std::shared_ptr<ThreadedQueue<Msg>> msgQueue);
    ~VolumeManager();

    bool init();
    void stop();
    void wait();

    void listSinks();

    void contextCallback(pa_context *c);
    void listSinksCallback(pa_context *c, const pa_sink_info *i, int eol);

    void lock();
    void unlock();

private:
    std::shared_ptr<ThreadedQueue<Msg>> _msgQueue;
    pa_threaded_mainloop *_mainloop;
    pa_context *_context;
    bool _isContextConnected;
};