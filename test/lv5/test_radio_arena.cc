#include <gtest/gtest.h>
#include "scoped_ptr.h"
#include "lv5/radio/arena.h"

TEST(RadioArenaCase, ArenaBlocksTest) {
  // check blocks size and iterator / const_iterator behavior
  iv::core::ScopedPtr<iv::lv5::radio::Arena> arena(new iv::lv5::radio::Arena);
  std::size_t index = 0;
  for (iv::lv5::radio::Arena::iterator it = arena->begin(), last = arena->end();
       it != last; ++it, ++index);
  EXPECT_EQ(iv::lv5::radio::kBlocks, index);
}
