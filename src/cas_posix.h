#ifndef IV_CAS_POSIX_H_
#define IV_CAS_POSIX_H_
#include "platform.h"
#include "thread.h"
namespace iv {
namespace core {
namespace thread {
#if defined(IV_COMPILER_GCC) && (IV_COMPILER_GCC > 40100)

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  return __sync_val_compare_and_swap(target, old_value, new_value);
}

#else

namespace detail {

static Mutex kCASMutex;

}  // namespace detail

inline int CompareAndSwap(volatile int* target,
                          int new_value, int old_value) {
  ScopedLock<Mutex> lock(&detail::kCASMutex);
  const int result = *target;
  if (result == old_value) {
    *target = new_value;
  }
  return result;
}

#endif

} } }  // namespace iv::core::thread
#endif  // IV_CAS_POSIX_H_
