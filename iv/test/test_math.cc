#include <gtest/gtest.h>
#include <iv/platform_math.h>

TEST(MathCase, TruncTest) {
  EXPECT_EQ(0.0, iv::core::math::Trunc(0.0));
  EXPECT_EQ(-0.0, iv::core::math::Trunc(-0.0));
}
