#include <gtest/gtest.h>
#include <algorithm>
#include <iv/scoped_ptr.h>
#include <iv/lv5/radio/arena.h>

TEST(RadioArenaCase, ArenaBlocksTest) {
  // check blocks size and iterator / const_iterator behavior
  iv::core::ScopedPtr<iv::lv5::radio::Arena> arena(new iv::lv5::radio::Arena(NULL));

  EXPECT_EQ(iv::lv5::radio::kBlocks, std::distance(arena->begin(), arena->end()));
  EXPECT_EQ(iv::lv5::radio::kBlocks, std::distance(arena->cbegin(), arena->cend()));
}
