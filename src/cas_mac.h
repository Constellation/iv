#ifndef _IV_CAS_MAC_H_
#define _IV_CAS_MAC_H_
#include <libkern/OSAtomic.h>
#include "thread.h"
namespace iv {
namespace core {
namespace thread {

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  int prev;
  do {
    if (OSAtomicCompareAndSwapInt(old_value, new_value, target)) {
     return old_value;
    }
    prev = *target;
  } while (prev == old_value);
  return prev;
}

} } }  // iv::core::thread
#endif  // _IV_CAS_MAC_H_
