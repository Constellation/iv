#ifndef _IV_CALLONCE_H_
#define _IV_CALLONCE_H_
#ifdef OS_WIN
#include <windows.h>
#endif
#include "cas.h"
namespace iv {
namespace core {
namespace thread {

enum OnceState {
  ONCE_INIT,
  ONCE_DONE
};

struct Once {
  Once() : state_(0), counter_(0) { }
#ifdef OS_WIN
  volatile LONG state_;
  volatile LONG counter_;
#else
  volatile int state_;
  volatile int counter_;
#endif
};

template<typename Func>
inline void CallOnce(Once* once, Func func) {
  if (once->state_ != ONCE_INIT) {
    return;
  }
  if (0 == CompareAndSwap(&(once->counter_), 1, 0)) {
    func();
    CompareAndSwap(&(once->state_), ONCE_DONE, ONCE_INIT);
  } else {
    // busy loop
    while (once->state_ == ONCE_INIT) {
      YieldCPU();
    }
  }
}

void ResetOnce(Once* once) {
  CompareAndSwap(&(once->state_), ONCE_INIT, ONCE_DONE);
  CompareAndSwap(&(once->counter_), 0, 1);
}

} } }  // iv::core::thread
#endif  // _IV_CALLONCE_H_
