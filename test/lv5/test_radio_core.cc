#include <gtest/gtest.h>
#include "scoped_ptr.h"
#include "lv5/radio/core.h"
#include "lv5/jsstring.h"

TEST(RadioCoreCase, MainTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  for (std::size_t i = 0; i < 100; ++i) {
    iv::lv5::radio::Cell* cell = core->Allocate<iv::lv5::JSString>();
    EXPECT_TRUE(cell);
  }
}

TEST(RadioCoreCase, BlockAddrTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  iv::lv5::radio::Cell* cell1 = core->Allocate<iv::lv5::JSString>();
  iv::lv5::radio::Cell* cell2 = core->Allocate<iv::lv5::JSString>();
  EXPECT_EQ(cell1->block(), cell2->block());
}
