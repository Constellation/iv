#ifndef _IV_CAS_MAC_H_
#define _IV_CAS_MAC_H_
#include <libkern/OSAtomic.h>
#include "thread.h"
namespace iv {
namespace core {
namespace thread {

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  return OSAtomicCompareAndSwapInt(old_value, new_value, target) ?
      old_value : *target;
}

} } }  // iv::core::thread
#endif  // _IV_CAS_MAC_H_
