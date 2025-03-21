#include "volume_manager.h"
#include "threaded_queue.h"
#include "msg.h"

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>

#include <iostream>
#include <memory>
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
    LOG("waiting for PulseAudio mainloop to finish...");
    pa_context_disconnect(_context);
    pa_threaded_mainloop_stop(_mainloop);
    pa_threaded_mainloop_wait(_mainloop);
    pa_context_unref(_context);
    pa_threaded_mainloop_free(_mainloop);
}

void VolumeManager::listSinks() {
    if (!_isContextConnected) return;
    lock();
    pa_context_get_sink_info_list(_context,
        [](pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
            VolumeManager* instance = static_cast<VolumeManager*>(userdata);
            if (instance) instance->listSinksCallback(c, i, eol);
        },
        this);
    unlock();
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
    if (eol) return;
    LOG("Sink " << i->index << ", name: " << i->name << ", desc: " << i->description);
    pa_cvolume vol = i->volume;
    for (int i = 0; i < vol.channels; i++) {
        LOG("  volume ch: " << i << ": " << vol.values[i]/65540.0*100 << "%");
    }
    LOG("  mute: " << i->mute);
}

void VolumeManager::lock() {
    pa_threaded_mainloop_lock(_mainloop);
}

void VolumeManager::unlock() {
    pa_threaded_mainloop_unlock(_mainloop);
}