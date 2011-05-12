#ifndef _IV_OS_ALLOCATOR_MAC_H_
#define _IV_OS_ALLOCATOR_MAC_H_
#include <mach/mach_error.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <malloc/malloc.h>
namespace iv {
namespace core {

inline void* OSAllocator::Allocate(std::size_t bytes) {
  vm_address_t mem;
  const kern_return_t err = vm_allocate(
      static_cast<vm_map_t>(mach_task_self()),
      &mem,
      static_cast<vm_size_t>(bytes),
      VM_FLAGS_ANYWHERE);
  if (err != KERN_SUCCESS) {
    std::abort();
  }
  assert(mem);
  return reinterpret_cast<void*>(mem);
}

inline void OSAllocator::Deallocate(void* address, std::size_t bytes) {
  const kern_return_t err = vm_deallocate(
      static_cast<vm_map_t>(mach_task_self()),
      reinterpret_cast<vm_address_t>(address),
      static_cast<vm_size_t>(bytes));
  if (err != KERN_SUCCESS) {
    std::abort();
  }
}

inline void OSAllocator::Commit(void* address, std::size_t bytes) { }

inline void OSAllocator::Decommit(void* addr, std::size_t bytes) { }

} }  // namespace iv::core
#endif  // _IV_OS_ALLOCATOR_MAC_H_
