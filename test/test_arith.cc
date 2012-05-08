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
