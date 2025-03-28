#include "volume_manager.h"
#include "threaded_queue.h"
#include "msg.h"
#include "session.h"

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>

#include <iostream>
#include <memory>
#include <cmath>
using namespace std;

VolumeManager::VolumeManager(shared_ptr<ThreadedQueue<Msg>> msgQueue)
    :_msgQueue(msgQueue) {
    _isContextConnected = false;
}

VolumeManager::~VolumeManager() {

}

bool VolumeManager::init() {
    _mainloop = pa_threaded_mainloop_new();
    _context = pa_context_new(pa_threaded_mainloop_get_api(_mainloop), APPLICATION_NAME);
    pa_context_set_state_callback(
        _context,
        [](pa_context* c, void* userdata) {
            VolumeManager* instance = static_cast<VolumeManager*>(userdata);
            if (instance) instance->contextCallback(c);
        },
        this);

    pa_threaded_mainloop_start(_mainloop);
    lock();
    pa_context_connect(_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    unlock();
    LOG("PulseAudio mainloop started");
    return true;
}

void VolumeManager::stop() {
    
}

void VolumeManager::wait() {
    LOG("stopping PulseAudio mainloop...");
    pa_context_disconnect(_context);
    pa_threaded_mainloop_stop(_mainloop);
    pa_threaded_mainloop_wait(_mainloop);
    pa_context_unref(_context);
    pa_threaded_mainloop_free(_mainloop);
}

void VolumeManager::listSinks(bool lock) {
    if (!_isContextConnected) return;
    if (lock) this->lock();
    _devicePool.clear();
    pa_context_get_sink_info_list(_context,
        [](pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
            VolumeManager* instance = static_cast<VolumeManager*>(userdata);
            if (instance) instance->listSinksCallback(c, i, eol);
        },
        this);
    if (lock) unlock();
}

void VolumeManager::contextCallback(pa_context *c) {
    switch (pa_context_get_state(c))
    {
    case PA_CONTEXT_READY:
        LOG("PulseAudio context READY");
        _isContextConnected = true;
        _msgQueue->pushAndSignal(Msg{MsgType::PA_CONTEXT_READY, "ready"});
        break;
    case PA_CONTEXT_FAILED:
        LOG("PulseAudio context FAILED");
        _isContextConnected = false;
        _msgQueue->pushAndSignal(Msg{MsgType::PA_CONTEXT_DISCONNECTED, "failed"});
        break;
    case PA_CONTEXT_TERMINATED:
        LOG("PulseAudio context TERMINATED");
        _isContextConnected = false;
        _msgQueue->pushAndSignal(Msg{MsgType::PA_CONTEXT_DISCONNECTED, "terminated"});
        break;
    
    default:
        break;
    }
}

void VolumeManager::listSinksCallback(pa_context *c, const pa_sink_info *i, int eol) {
    if (eol) {
        for (Session &dev : _devicePool) {
            LOG(" device index #" << dev.index);
            LOG("  name: " << dev.name);
            LOG("  desc: " << dev.description);
            LOG("  volume: "<< dev.volume);
            LOG("  mute: " << dev.muted);
        }
        LOG("--total devices: " << _devicePool.size());
        return;
    }
    pa_cvolume vol = i->volume;
    Session dev;
    dev.pid = -1;
    dev.index = i->index;
    dev.name = i->name;
    dev.description = i->description;
    if (vol.channels >= 2) dev.volume = (vol.values[0] + vol.values[1]) / 2;
    else if (vol.channels >= 1) dev.volume = vol.values[0];
    else dev.volume = 0;
    dev.volume = floor((dev.volume / ((double)PA_VOLUME_NORM)) * 100.0) / 100.0;
    dev.muted = i->mute ? true : false;
    _devicePool.push_back(dev);
}

void VolumeManager::lock() {
    pa_threaded_mainloop_lock(_mainloop);
}

void VolumeManager::unlock() {
    pa_threaded_mainloop_unlock(_mainloop);
}