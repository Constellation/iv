#ifndef _IV_THREAD_WIN_H_
#define _IV_THREAD_WIN_H_
#include <windows.h>
namespace iv {
namespace core {
namespace thread {

class WinMutex : private Noncopyable<WinMutex>::type {
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

} } }  // namespace iv::core::thread
#endif  // _IV_THREAD_WIN_H_
