#include <gtest/gtest.h>
#include <limits>
#include <cmath>
#include "lv5/jsval.h"

TEST(JSValCase, NaNTest) {
  using iv::lv5::JSVal;
  JSVal value = std::numeric_limits<double>::quiet_NaN();
  ASSERT_TRUE(value.IsNumber());
  EXPECT_TRUE(std::isnan(value.number()));
}
