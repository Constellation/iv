#ifndef IV_OS_ALLOCATOR_WIN_H_
#define IV_OS_ALLOCATOR_WIN_H_
#include <windows.h>
namespace iv {
namespace core {

inline void* OSAllocator::Allocate(std::size_t bytes) {
#if 1
  return malloc(bytes);
#else
  void* mem = VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!mem) {
    std::abort();
  }
  return mem;
#endif
}

inline void OSAllocator::Deallocate(void* address, std::size_t bytes) {
#if 1
  free(address);
#else
  const bool res = VirtualFree(address, 0, MEM_RELEASE) != 0;
  if (!res) {
    std::abort();
  }
#endif
}

inline void OSAllocator::Commit(void* address, std::size_t bytes) {
#if 1
  return;
#else
  const void* mem = VirtualAlloc(address, bytes, MEM_COMMIT, PAGE_READWRITE);
  if (!mem) {
    std::abort();
  }
#endif
}

inline void OSAllocator::Decommit(void* address, std::size_t bytes) {
#if 1
  return;
#else
  const bool res = VirtualFree(address, bytes, MEM_DECOMMIT) != 0;
  if (!res) {
    std::abort();
  }
#endif
}

} }  // namespace iv::core
#endif  // IV_OS_ALLOCATOR_WIN_H_
