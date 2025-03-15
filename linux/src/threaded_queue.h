#pragma once
#include "globals.h"
#include <queue>
#include <pthread.h>

template <typename T>
class ThreadedQueue {

public:
    ThreadedQueue();
    ~ThreadedQueue();

    void lock();
    void unlock();
    void wait();

    bool empty();
    int size();
    T front();
    T back();
    void push(T element);
    void pop();
    void pushAndSignal(T element);

private:
    std::queue<T> _queue;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
};

