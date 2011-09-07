#ifndef IV_ATOMIC_H_
#define IV_ATOMIC_H_
#include "platform.h"

#if defined(IV_OS_WIN)
#include <windows.h>
#elif defined(IV_OS_MACOSX)
#include <libkern/OSAtomic.h>
#endif

namespace iv {
namespace core {

#if defined(IV_COMPILER_GCC) || defined(IV_COMPILER_CLANG)
inline int AtomicIncrement(volatile int* target) {
  return __sync_add_and_fetch(target, 1);
}
inline int AtomicDecrement(volatile int* target) {
  return __sync_sub_and_fetch(target, 1);
}
#elif defined(IV_OS_WIN)
inline int AtomicIncrement(volatile int* target) {
  return InterlockedIncrement(reinterpret_cast<volatile long*>(target));
}
inline int AtomicDecrement(volatile int* target) {
  return InterlockedDecrement(reinterpret_cast<volatile long*>(target));
}
#elif defined(IV_OS_MACOSX)
inline int AtomicIncrement(volatile int* target) {
  return OSAtomicIncrement64Barrier(const_cast<int*>(target));
}
inline int AtomicDecrement(volatile int* target) {
  return OSAtomicDecrement64Barrier(const_cast<int*>(target));
}
#else
#error ATOMIC INCREMENT / DECREMENT NOT DEFINED in atomic.h
#endif

} }  // namespace iv::core
#endif  // IV_ATOMIC_H_
