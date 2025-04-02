#include "threaded_queue.h"
#include "msg.h"

#include <queue>
#include <string>
#include <pthread.h>

using namespace std;

#define TEMPL template <typename T>

template class ThreadedQueue<Msg>;

TEMPL ThreadedQueue<T>::ThreadedQueue() {
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
}

TEMPL ThreadedQueue<T>::~ThreadedQueue() {

}

TEMPL void ThreadedQueue<T>::lock() {
    pthread_mutex_lock(&_mutex);
}

TEMPL void ThreadedQueue<T>::unlock() {
    pthread_mutex_unlock(&_mutex);
}

TEMPL void ThreadedQueue<T>::wait() {
    if (!empty()) return;
    pthread_cond_wait(&_cond, &_mutex);
}

TEMPL bool ThreadedQueue<T>::empty() {
    return _queue.empty();
}

TEMPL int ThreadedQueue<T>::size() {
    return _queue.size();
}

TEMPL T ThreadedQueue<T>::front() {
    return _queue.front();
}

TEMPL T ThreadedQueue<T>::back() {
    return _queue.back();
}

TEMPL void ThreadedQueue<T>::push(T element) {
    _queue.push(element);
}

TEMPL void ThreadedQueue<T>::pop() {
    _queue.pop();
}

TEMPL void ThreadedQueue<T>::pushAndSignal(T element) {
    lock();
    push(element);
    pthread_cond_signal(&_cond);
    unlock();
}



