#ifndef _IV_LV5_OS_ALLOCATOR_H_
#define _IV_LV5_OS_ALLOCATOR_H_
#include "os_defines.h"
namespace iv {
namespace lv5 {

class OSAllocator {
 public:
  static void* Allocate(std::size_t bytes);
  static void Deallocate(void* address, std::size_t bytes);
  static void Commit(void* address, std::size_t bytes);
  static void Decommit(void* addr, std::size_t bytes);
};

} }  // namespace iv::lv5
#ifdef OS_WIN
#include "windows.h"
#include "os_allocator_win.h"
#else
#include "os_allocator_posix.h"
#endif  // OS_WIN
#endif  // _IV_LV5_OS_ALLOCATOR_H_
