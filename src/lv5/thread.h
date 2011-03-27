#ifndef _IV_LV5_THREAD_H_
#define _IV_LV5_THREAD_H_
#include <cassert>
#include "os_defines.h"
#include "noncopyable.h"
namespace iv {
namespace lv5 {
namespace thread {

template<typename Mutex>
class ScopedLock : private core::Noncopyable<ScopedLock<Mutex> >::type {
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



} } }  // iv::lv5::thread
#if defined(OS_WIN)
#include "lv5/thread_win.h"
#else
#include "lv5/thread_posix.h"
#endif  // OS_WIN
#endif  // _IV_LV5_THREAD_H_
