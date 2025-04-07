#include "volume_manager.h"
#include "threaded_queue.h"
#include "msg.h"
#include "session.h"

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>

#include <iostream>
#include <memory>
#include <cmath>
#include <string>
#include <sstream>
#include <map>
using namespace std;

VolumeManager::VolumeManager(shared_ptr<ThreadedQueue<Msg>> msgQueue)
    :_msgQueue(msgQueue) {
    _isContextConnected = false;
    _listSinksInProgress = false;
    _listSinkInputsInProgress = false;
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
            if (instance) instance->_contextCallback(c);
        },
        this
    );
    
    pa_context_set_subscribe_callback(
        _context,
        [](pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
            VolumeManager* instance = static_cast<VolumeManager*>(userdata);
            if (instance) instance->_subscribeCallback(c, t, idx);
        },
        this
    );

    pa_threaded_mainloop_start(_mainloop);
    pa_context_connect(_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    LOG("PulseAudio mainloop started");
    return true;
}

void VolumeManager::stop() {
    
}

void VolumeManager::wait() {
    LOG("stopping PulseAudio mainloop...");
    pa_context_subscribe(_context, (pa_subscription_mask_t)NULL, NULL, NULL);
    pa_context_disconnect(_context);
    pa_threaded_mainloop_stop(_mainloop);
    pa_context_unref(_context);
    pa_threaded_mainloop_free(_mainloop);
}

void VolumeManager::listSinks(bool lck, bool wait) {
    if (!_isContextConnected) return;
    if (_listSinksInProgress) return;
    if (lck) lock();
    _listSinksInProgress = true;
    _devicePool.clear();
    pa_operation *op = pa_context_get_sink_info_list(_context,
        [](pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
            VolumeManager* instance = static_cast<VolumeManager*>(userdata);
            if (instance) instance->_listSinksCallback(c, i, eol);
        },
        this
    );
    pa_operation_unref(op);
    if (wait) while (_listSinksInProgress) pa_threaded_mainloop_wait(_mainloop);
    if (lck) unlock();
}

void VolumeManager::listSinkInputs(bool lck, bool wait) {
    if (!_isContextConnected) return;
    if (_listSinkInputsInProgress) return;
    if (lck) lock();
    _listSinkInputsInProgress = true;
    _sessionPool.clear();
    pa_operation *op = pa_context_get_sink_input_info_list(_context,
        [](pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) {
            VolumeManager* instance = static_cast<VolumeManager*>(userdata);
            if (instance) instance->_listSinkInputsCallback(c, i, eol);
        },
        this
    );
    pa_operation_unref(op);
    if (wait) while (_listSinkInputsInProgress) pa_threaded_mainloop_wait(_mainloop);
    if (lck) unlock();
}

void VolumeManager::_contextCallback(pa_context *c) {
    switch (pa_context_get_state(c))
    {
    case PA_CONTEXT_READY:
        LOG("PulseAudio context READY");
        _isContextConnected = true;
        {
            pa_operation *op = pa_context_subscribe(
                _context,
                (pa_subscription_mask_t) (PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SINK_INPUT),
                [](pa_context *c, int success, void *userdata) {
                    if (success) {
                        LOG("subscribed to sink input events");
                    } else {
                        LOG("failed to subscribe to events");
                    }
                },
                nullptr
            );
            pa_operation_unref(op);
        }
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

void VolumeManager::_listSinksCallback(pa_context *c, const pa_sink_info *i, int eol) {
    if (eol) {
        _listSinksInProgress = false;
        pa_threaded_mainloop_signal(_mainloop, 0);
        stringstream ss;
        for (auto &entry : _devicePool) {
            Session &dev = entry.second;
            ss << " device index #" << dev.index << "\n";
            ss << "  name: " << dev.name << "\n";
            ss << "  desc: " << dev.description << "\n";
            ss << "  volume: "<< dev.volume << "\n";
            ss << "  mute: " << dev.muted << "\n";
        }
        ss << "--total devices: " << _devicePool.size() << "\n";
        _msgQueue->pushAndSignal(Msg{MsgType::LIST_SINKS_COMPLETE, ss.str()});
        return;
    }
    _processSink(i);
}

void VolumeManager::_processSink(const pa_sink_info *i) {
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

    _devicePool[dev.index] = dev;
}

void VolumeManager::_removeSink(int idx) {
    _devicePool.erase(idx);
}

void VolumeManager::_listSinkInputsCallback(pa_context *c, const pa_sink_input_info *i, int eol) {
    if (eol) {
        _listSinkInputsInProgress = false;
        pa_threaded_mainloop_signal(_mainloop, 0);
        stringstream ss;
        for (auto &entry : _sessionPool) {
            Session &session = entry.second;
            ss << " sink input index #" << session.index << "\n";
            ss << "  name: " << session.name << "\n";
            ss << "  desc: " << session.description << "\n";
            ss << "  volume: "<< session.volume << "\n";
            ss << "  mute: " << session.muted << "\n";
        }
        ss << "--total sink inputs: " << _sessionPool.size() << "\n";
        _msgQueue->pushAndSignal(Msg{MsgType::LIST_SINK_INPUTS_COMPLETE, ss.str()});
        return;
    }
    _processSinkInput(i);
}

void VolumeManager::_processSinkInput(const pa_sink_input_info *i) {
    pa_cvolume vol = i->volume;
    Session session;
    session.pid = -1; // TODO
    session.index = i->index;
    session.name = pa_proplist_gets(i->proplist, "application.name");
    session.description = "-";
    if (vol.channels >= 2) session.volume = (vol.values[0] + vol.values[1]) / 2;
    else if (vol.channels >= 1) session.volume = vol.values[0];
    else session.volume = 0;
    session.volume = floor((session.volume / ((double)PA_VOLUME_NORM)) * 100.0) / 100.0;
    session.muted = i->mute ? true : false;

    _sessionPool[session.index] = session;
}

void VolumeManager::_removeSinkInput(int idx) {
    _sessionPool.erase(idx);
}


void VolumeManager::_subscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx) {
    pa_subscription_event_type_t facility = (pa_subscription_event_type_t) (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK);
    pa_subscription_event_type_t eventType = (pa_subscription_event_type_t) (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK);

    if (facility == PA_SUBSCRIPTION_EVENT_SINK) {

        if (eventType == PA_SUBSCRIPTION_EVENT_NEW) {
            pa_operation *op = pa_context_get_sink_info_by_index(
                _context,
                idx,
                [](pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
                    if (eol) return;
                    VolumeManager *instance = static_cast<VolumeManager*>(userdata);
                    if (!instance) return;
                    instance->_processSink(i);
                    instance->_msgQueue->pushAndSignal(Msg{MsgType::SINK_ADDED, to_string(i->index)});
                },
                this
            );
            pa_operation_unref(op);
        }
        else if (eventType == PA_SUBSCRIPTION_EVENT_CHANGE) {
            pa_operation *op = pa_context_get_sink_info_by_index(
                _context,
                idx,
                [](pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
                    if (eol) return;
                    VolumeManager *instance = static_cast<VolumeManager*>(userdata);
                    if (!instance) return;
                    instance->_processSink(i);
                    instance->_msgQueue->pushAndSignal(Msg{MsgType::SINK_CHANGED, to_string(i->index)});
                },
                this
            );
            pa_operation_unref(op);
        }
        else if (eventType == PA_SUBSCRIPTION_EVENT_REMOVE) {
            _removeSink(idx);
            _msgQueue->pushAndSignal(Msg{MsgType::SINK_REMOVED, to_string(idx)});
        }
    }
    else if (facility == PA_SUBSCRIPTION_EVENT_SINK_INPUT) {

        if (eventType == PA_SUBSCRIPTION_EVENT_NEW) {
            pa_operation *op = pa_context_get_sink_input_info(
                _context,
                idx,
                [](pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) {
                    if (eol) return;
                    VolumeManager *instance = static_cast<VolumeManager*>(userdata);
                    if (!instance) return;
                    instance->_processSinkInput(i);
                    instance->_msgQueue->pushAndSignal(Msg{MsgType::SINK_INPUT_ADDED, to_string(i->index)});
                },
                this
            );
            pa_operation_unref(op);
        }
        else if (eventType == PA_SUBSCRIPTION_EVENT_CHANGE) {
            pa_operation *op = pa_context_get_sink_input_info(
                _context,
                idx,
                [](pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) {
                    if (eol) return;
                    VolumeManager *instance = static_cast<VolumeManager*>(userdata);
                    if (!instance) return;
                    instance->_processSinkInput(i);
                    instance->_msgQueue->pushAndSignal(Msg{MsgType::SINK_INPUT_CHANGED, to_string(i->index)});
                },
                this
            );
            pa_operation_unref(op);
        }
        else if (eventType == PA_SUBSCRIPTION_EVENT_REMOVE) {
            _removeSinkInput(idx);
            _msgQueue->pushAndSignal(Msg{MsgType::SINK_INPUT_REMOVED, to_string(idx)});
        }
    }

}

void VolumeManager::lock() {
    pa_threaded_mainloop_lock(_mainloop);
}

void VolumeManager::unlock() {
    pa_threaded_mainloop_unlock(_mainloop);
}

void VolumeManager::listSinks_sync() {
    listSinks(true, true);
}

void VolumeManager::listSinkInputs_sync() {
    listSinkInputs(true, true);
}

map<int, Session> *VolumeManager::getSessionPool() {
    return &_sessionPool;
}

map<int, Session> *VolumeManager::getDevicePool() {
    return &_devicePool;
}

void VolumeManager::setSinkVolume(int index, float volume, bool lck) {
    if (lck) lock();
    if (_devicePool.count(index)) {
        pa_cvolume vol;
        pa_cvolume_set(&vol, 2, volume * PA_VOLUME_NORM);

        pa_operation *op = pa_context_set_sink_volume_by_index(_context, index, &vol, NULL, NULL);
        pa_operation_unref(op);
    }
    if (lck) unlock();
}

void VolumeManager::setSinkMute(int index, bool mute, bool lck) {
    if (lck) lock();
    if (_devicePool.count(index)) {
        pa_operation *op = pa_context_set_sink_mute_by_index(_context, index, mute ? 1 : 0, NULL, NULL);
        pa_operation_unref(op);
    }
    if (lck) unlock();
}

void VolumeManager::setSinkInputVolume(int index, float volume, bool lck) {
    if (lck) lock();
    if (_sessionPool.count(index)) {
        pa_cvolume vol;
        pa_cvolume_set(&vol, 2, volume * PA_VOLUME_NORM);

        pa_operation *op = pa_context_set_sink_input_volume(_context, index, &vol, NULL, NULL);
        pa_operation_unref(op);
    }
    if (lck) unlock();
}

void VolumeManager::setSinkInputMute(int index, bool mute, bool lck) {
    if (lck) lock();
    if (_sessionPool.count(index)) {
        pa_operation *op = pa_context_set_sink_mute_by_index(_context, index, mute ? 1 : 0, NULL, NULL);
        pa_operation_unref(op);
    }
    if (lck) unlock();
}