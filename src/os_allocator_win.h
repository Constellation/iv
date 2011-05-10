#ifndef _IV_OS_ALLOCATOR_WIN_H_
#define _IV_OS_ALLOCATOR_WIN_H_
#include "windows.h"
namespace iv {
namespace core {

inline void* OSAllocator::Allocate(std::size_t bytes) {
  void* mem = VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!mem) {
    std::abort();
  }
  return mem;
}

inline void OSAllocator::Deallocate(void* address, std::size_t bytes) {
  const bool res = VirtualFree(address, bytes, MEM_RELEASE);
  if (!res) {
    std::abort();
  }
}

inline void OSAllocator::Commit(void* address, std::size_t bytes) {
  const void* mem = VirtualAlloc(address, bytes, MEM_COMMIT, PAGE_READWRITE);
  if (!mem) {
    std::abort();
  }
}

inline void OSAllocator::Decommit(void* addr, std::size_t bytes) {
  const bool res = VirtualFree(address, bytes, MEM_DECOMMIT);
  if (!res) {
    std::abort();
  }
}

} }  // namespace iv::core
#endif  // _IV_OS_ALLOCATOR_WIN_H_
