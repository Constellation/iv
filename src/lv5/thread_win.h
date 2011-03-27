#ifndef _IV_LV5_THREAD_WIN_H_
#define _IV_LV5_THREAD_WIN_H_
#include <windows.h>
namespace iv {
namespace lv5 {
namespace thread {

class WinMutex : private core::Noncopyable<WinMutex>::type {
 public:
  WinMutex() {
    ::InitializeCriticalSection(&mutex_);
  }

  ~WinMutex() {
    ::DeleteCriticalSection(&mutex);
  }

  int Lock() {
    ::EnterCriticalSection(&mutex);
    return 0;
  }

  int Unlock() {
    ::LeaveCriticalSection(&mutex);
    return 0;
  }

  bool TryLock() {
    return ::TryEnterCriticalSection(&mutex_);
  }

 private:
  CRITICAL_SECTION mutex_;
};

typedef WinMutex Mutex;

} } }  // namespace iv::lv5::thread
#endif  // _IV_LV5_THREAD_WIN_H_
