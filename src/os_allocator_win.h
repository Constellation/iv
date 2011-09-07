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
printf("alloc mem=%p, size=%d\n", mem, bytes);
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
printf("free addr=%p, size=%d\n", address, bytes);
  const bool res = VirtualFree(address, bytes, MEM_RELEASE) != 0;
  printf("last error=%d\n", ::GetLastError());
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
printf("commit mem=%p, addr=%p, byte=%d\n", mem, address, bytes);
  if (!mem) {
    std::abort();
  }
#endif
}

inline void OSAllocator::Decommit(void* address, std::size_t bytes) {
#if 1
  return;
#else
printf("decomit addr=%p, byte=%d\n", address, bytes);
  const bool res = VirtualFree(address, bytes, MEM_DECOMMIT) != 0;
  if (!res) {
    std::abort();
  }
#endif
}

} }  // namespace iv::core
#endif  // IV_OS_ALLOCATOR_WIN_H_
