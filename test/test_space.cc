#include <gtest/gtest.h>
#include "detail/cstdint.h"
#include "scoped_ptr.h"
#include "alloc.h"

TEST(SpaceCase, AllocateMemoryAlignemntTest) {
  iv::core::ScopedPtr<iv::core::Space> space(new iv::core::Space());
  {
    uintptr_t mem = reinterpret_cast<uintptr_t>(space->New(3));
    ASSERT_EQ(0, mem % iv::core::Arena::kAlignment);
  }
  {
    uintptr_t mem = reinterpret_cast<uintptr_t>(space->New(3));
    ASSERT_EQ(0, mem % iv::core::Arena::kAlignment);
  }
  {
    uintptr_t mem = reinterpret_cast<uintptr_t>(space->New(3));
    ASSERT_EQ(0, mem % iv::core::Arena::kAlignment);
  }
  {
    uintptr_t mem = reinterpret_cast<uintptr_t>(space->New(1000));
    ASSERT_EQ(0, mem % iv::core::Arena::kAlignment);
  }
  {
    uintptr_t mem = reinterpret_cast<uintptr_t>(space->New(100));
    ASSERT_EQ(0, mem % iv::core::Arena::kAlignment);
  }
  {
    for (std::size_t i = 0; i < 1000000; ++i) {
      uintptr_t mem = reinterpret_cast<uintptr_t>(space->New(23));
      ASSERT_EQ(0, mem % iv::core::Arena::kAlignment);
    }
  }
}
