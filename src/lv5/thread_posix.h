#ifndef _IV_LV5_THREAD_POSIX_H_
#define _IV_LV5_THREAD_POSIX_H_
#include <cassert>
#include <cstdlib>
#include <pthread.h>
#include "noncopyable.h"
namespace iv {
namespace lv5 {
namespace thread {

class PosixMutex : private core::Noncopyable<PosixMutex>::type {
 public:
  PosixMutex() {
    pthread_mutexattr_t attrs;
    int result = pthread_mutexattr_init(&attrs);
    assert(result == 0);
    result = pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    assert(result == 0);
    result = pthread_mutex_init(&mutex_, &attrs);
    assert(result == 0);
  }

  ~PosixMutex() {
    pthread_mutex_destroy(&mutex_);
  }

  int Lock() {
    const int res = pthread_mutex_lock(&mutex_);
    return res;
  }

  int Unlock() {
    const int res = pthread_mutex_unlock(&mutex_);
    return res;
  }

  bool TryLock() {
    const int res = pthread_mutex_trylock(&mutex_);
    if (res == EBUSY) {
      return false;
    }
    assert(res == 0);
    return true;
  }

 private:
  pthread_mutex_t mutex_;
};

typedef PosixMutex Mutex;

} } }  // namespace iv::lv5::thread
#endif  // _IV_LV5_THREAD_POSIX_H_
