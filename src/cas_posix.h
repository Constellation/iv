#ifndef _IV_CAS_POSIX_H_
#define _IV_CAS_POSIX_H_
#include "platform.h"
#include "thread.h"
namespace iv {
namespace core {
namespace thread {

#if defined(__GNUC__) && (__GNUC_VERSION__ > 40100)

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  return __sync_val_compare_and_swap(target, old_value, new_value);
}

#else

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

#endif

} } }  // iv::core::thread
#endif  // _IV_CAS_POSIX_H_
