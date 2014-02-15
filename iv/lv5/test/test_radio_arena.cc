#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <iv/lv5/radio/arena.h>

TEST(RadioArenaCase, ArenaBlocksTest) {
  // check blocks size and iterator / const_iterator behavior
  std::unique_ptr<iv::lv5::radio::Arena> arena(new iv::lv5::radio::Arena(nullptr));

  EXPECT_EQ(iv::lv5::radio::kBlocks,
            static_cast<std::size_t>(std::distance(arena->begin(), arena->end())));
  EXPECT_EQ(iv::lv5::radio::kBlocks,
            static_cast<std::size_t>(std::distance(arena->cbegin(), arena->cend())));
}
