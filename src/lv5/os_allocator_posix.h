#ifndef _IV_LV5_OS_ALLOCATOR_POSIX_H_
#define _IV_LV5_OS_ALLOCATOR_POSIX_H_
#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <sys/mman.h>
namespace iv {
namespace lv5 {

inline void* OSAllocator::Allocate(std::size_t bytes) {
  int fd = -1;
  void* mem = NULL;
  mem = mmap(mem, bytes,
             PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, fd, 0);
  if (mem == MAP_FAILED) {
    std::abort();
  }
  return mem;
}

inline void OSAllocator::Deallocate(void* address, std::size_t bytes) {
  const int res = munmap(address, bytes);
  if (res == -1) {
    std::abort();
  }
}

inline void OSAllocator::Commit(void* address, std::size_t bytes) {
#ifdef MADV_FREE_REUSE
  while (madvise(address, bytes, MADV_FREE_REUSE) == -1 && errno == EAGAIN) {
  }
#endif
}

inline void OSAllocator::Decommit(void* addr, std::size_t bytes) {
#if defined(MADV_FREE_REUSABLE)
  while (madvise(addr, bytes, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN) {
  }
#elif defined(MADV_FREE)
  while (madvise(addr, bytes, MADV_FREE) == -1 && errno == EAGAIN) { }
#elif defined(MADV_DONTNEED)
  while (madvise(addr, bytes, MADV_DONTNEED) == -1 && errno == EAGAIN) { }
#endif
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_OS_ALLOCATOR_POSIX_H_
