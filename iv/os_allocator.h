#ifndef IV_OS_ALLOCATOR_H_
#define IV_OS_ALLOCATOR_H_
#include <iv/platform.h>
namespace iv {
namespace core {

class OSAllocator {
 public:
  static void* Allocate(std::size_t bytes);
  static void Deallocate(void* address, std::size_t bytes);
  static void Commit(void* address, std::size_t bytes);
  static void Decommit(void* addr, std::size_t bytes);
};

} }  // namespace iv::core
#if defined(IV_OS_WIN)
#include <iv/os_allocator_win.h>
#else
#include <iv/os_allocator_posix.h>
#endif  // IV_OS_WIN
#endif  // IV_OS_ALLOCATOR_H_
