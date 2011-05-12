#ifndef _IV_CAS_POSIX_H_
#define _IV_CAS_POSIX_H_
#include "thread.h"
namespace iv {
namespace core {
namespace thread {

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  static Mutex mutex;
  ScopedLock<Mutex> lock(&mutex);
  const int result = *target;
  if (result == old_value) {
    *target = new_value;
  }
  return result;
}

} } }  // iv::core::thread
#endif  // _IV_CAS_POSIX_H_
