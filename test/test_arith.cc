#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/arith.h>

TEST(ArithCase, FLP2Test) {
  typedef std::array<uint32_t, 64> Samples;
  const Samples expected = { {
    0,
    1,
    2, 2,
    4, 4, 4, 4,
    8, 8, 8, 8, 8, 8, 8, 8,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
  } };
  uint32_t i = 0;
  for (Samples::const_iterator it = expected.begin(),
       last = expected.end(); it != last; ++it, ++i) {
    EXPECT_EQ(*it, iv::core::math::FLP2(i)) << i;
  }
}

TEST(ArithCase, CLP2Test) {
  typedef std::array<uint32_t, 49> Samples;
  const Samples expected = { {
    0,
    1,
    2,
    4, 4,
    8, 8, 8, 8,
    16, 16, 16, 16, 16, 16, 16, 16,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
  } };
  uint32_t i = 0;
  for (Samples::const_iterator it = expected.begin(),
       last = expected.end(); it != last; ++it, ++i) {
    EXPECT_EQ(*it, iv::core::math::CLP2(i)) << i;
  }
}

TEST(ArithCase, CLZTest) {
  EXPECT_EQ(32u, iv::core::math::CLZ(0));
  EXPECT_EQ(31u, iv::core::math::CLZ(1));
  EXPECT_EQ(30u, iv::core::math::CLZ(2));
  EXPECT_EQ(30u, iv::core::math::CLZ(3));
  EXPECT_EQ(29u, iv::core::math::CLZ(4));
  EXPECT_EQ(29u, iv::core::math::CLZ(5));
  EXPECT_EQ(29u, iv::core::math::CLZ(6));
  EXPECT_EQ(29u, iv::core::math::CLZ(7));
  EXPECT_EQ(28u, iv::core::math::CLZ(8));
  EXPECT_EQ(28u, iv::core::math::CLZ(15));
  EXPECT_EQ(27u, iv::core::math::CLZ(16));
  EXPECT_EQ(0u, iv::core::math::CLZ(UINT32_MAX));
}
