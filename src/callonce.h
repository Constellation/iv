#ifndef IV_CALLONCE_H_
#define IV_CALLONCE_H_
#include "platform.h"
#ifdef IV_OS_WIN
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
#ifdef IV_OS_WIN
  typedef volatile LONG int_type;
#else
  typedef volatile int int_type;
#endif
  Once() : state_(0), counter_(0) { }

  int_type state_;
  int_type counter_;
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

inline void ResetOnce(Once* once) {
  CompareAndSwap(&(once->state_), ONCE_INIT, ONCE_DONE);
  CompareAndSwap(&(once->counter_), 0, 1);
}

} } }  // iv::core::thread
#endif  // IV_CALLONCE_H_
