#include <tr1/cstdint>
#include <gtest/gtest.h>
#include "unicode.h"

namespace c = iv::core::unicode;

TEST(UnicodeCase, MaskTest) {
  const uint32_t ch = 0xFFFF;
  EXPECT_EQ(0xF, c::Mask<4>(ch));
  EXPECT_EQ(0xFF, c::Mask<8>(ch));
  EXPECT_EQ(0xFFF, c::Mask<12>(ch));
  EXPECT_EQ(0xFFFF, c::Mask<16>(ch));
}
