#ifndef _IV_THREAD_H_
#define _IV_THREAD_H_
#include <cassert>
#include "platform.h"
#include "noncopyable.h"
namespace iv {
namespace core {
namespace thread {

template<typename Mutex>
class ScopedLock : private Noncopyable<ScopedLock<Mutex> >::type {
 public:
  explicit ScopedLock(Mutex* mutex)
    : mutex_(mutex),
      locked_(false) {
    Lock();
  }

  ~ScopedLock() {
    if (locked_) {
      mutex_->Unlock();
    }
  }

 private:
  void Lock() {
    assert(!locked_);
    mutex_->Lock();
    locked_ = true;
  }

  Mutex* mutex_;
  bool locked_;
};

} } }  // iv::core::thread
#if defined(OS_WIN)
#include "thread_win.h"
#else
#include "thread_posix.h"
#endif  // OS_WIN
#endif  // _IV_LV5_THREAD_H_
