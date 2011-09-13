#include <gtest/gtest.h>
#include "os_allocator.h"
#include "utils.h"

TEST(OSAllocatorCase, SmallAllocateAndDeallocateTest) {
  // small memory
  {
    void* mem = iv::core::OSAllocator::Allocate(0xF);
    iv::core::OSAllocator::Deallocate(mem, 0xF);
  }

  {
    void* mem = iv::core::OSAllocator::Allocate(0x1);
    iv::core::OSAllocator::Deallocate(mem, 0x1);
  }

  {
    void* mem = iv::core::OSAllocator::Allocate(0xFF);
    iv::core::OSAllocator::Deallocate(mem, 0xFF);
  }
}

TEST(OSAllocatorCase, BigAllocateAndDeallocateTest) {
  // big memory
  {
    void* mem = iv::core::OSAllocator::Allocate(iv::core::Size::KB * 4);
    iv::core::OSAllocator::Deallocate(mem, iv::core::Size::KB * 4);
  }

  {
    void* mem = iv::core::OSAllocator::Allocate(iv::core::Size::MB * 4);
    iv::core::OSAllocator::Deallocate(mem, iv::core::Size::MB * 4);
  }

  {
    void* mem = iv::core::OSAllocator::Allocate(iv::core::Size::MB * 8);
    iv::core::OSAllocator::Deallocate(mem, iv::core::Size::MB * 8);
  }
}
