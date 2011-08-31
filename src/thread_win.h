#ifndef IV_THREAD_WIN_H_
#define IV_THREAD_WIN_H_
#include <windows.h>
namespace iv {
namespace core {
namespace thread {

class WinMutex : private Noncopyable<> {
 public:
  WinMutex() {
    ::InitializeCriticalSection(&mutex_);
  }

  ~WinMutex() {
    ::DeleteCriticalSection(&mutex_);
  }

  int Lock() {
    ::EnterCriticalSection(&mutex_);
    return 0;
  }

  int Unlock() {
    ::LeaveCriticalSection(&mutex_);
    return 0;
  }

  bool TryLock() {
    return ::TryEnterCriticalSection(&mutex_);
  }

 private:
  CRITICAL_SECTION mutex_;
};

typedef WinMutex Mutex;

inline void YieldCPU() {
  ::YieldProcessor();
}

} } }  // namespace iv::core::thread
#endif  // IV_THREAD_WIN_H_
