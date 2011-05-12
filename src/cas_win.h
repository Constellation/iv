#ifndef _IV_CAS_WIN_H_
#define _IV_CAS_WIN_H_
#include "windows.h"
namespace iv {
namespace core {
namespace thread {

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  return InterlockedCompareAndExchange(target, new_value, old_value);
}

} } }  // iv::core::thread
#endif  // _IV_CAS_WIN_H_
