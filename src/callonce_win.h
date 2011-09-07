#ifndef IV_CALLONCE_WIN_H_
#define IV_CALLONCE_WIN_H_
#include "cas.h"
#include "thread.h"

namespace iv {
namespace core {
namespace thread {

enum OnceState {
  ONCE_INIT,
  ONCE_DONE
};

#define IV_ONCE_INIT\
  { ::iv::core::thread::ONCE_INIT, ::iv::core::thread::ONCE_INIT }

struct Once {
#ifdef IV_OS_WIN
  typedef volatile LONG int_type;
#else
  typedef volatile int int_type;
#endif
  int_type state_;
  int_type counter_;
};

inline void CallOnce(Once* once, OnceCallback func) {
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

// not defined in pthread version
inline void ResetOnce(Once* once) {
  CompareAndSwap(&(once->state_), ONCE_INIT, ONCE_DONE);
  CompareAndSwap(&(once->counter_), 0, 1);
}

} } }  // iv::core::thread
#endif  // IV_CALLONCE_WIN_H_
