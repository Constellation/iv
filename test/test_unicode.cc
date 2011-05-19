#include <tr1/cstdint>
#include <string>
#include <iterator>
#include <algorithm>
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

TEST(UnicodeCase, UTF8ToUCS4) {
  const std::string str = "こんにちは";
  std::vector<uint32_t> actual;
  const std::tr1::array<uint32_t, 5> expect = { { 12371, 12435, 12395, 12385, 12399 } };
  c::detail::UTF8ToUCS4(str.begin(), str.end(), std::back_inserter(actual));
  EXPECT_TRUE(std::equal(expect.begin(), expect.end(), actual.begin()));
}

TEST(UnicodeCase, UTF8ToUTF16) {
  {
    const std::string str = "こんにちは";
    std::vector<uint16_t> actual;
    const std::tr1::array<uint16_t, 5> expect = { { 12371, 12435, 12395, 12385, 12399 } };
    c::detail::UTF8ToUTF16(str.begin(), str.end(), std::back_inserter(actual));
    EXPECT_TRUE(std::equal(expect.begin(), expect.end(), actual.begin()));
  }
  {
    // surrogate pair test
    const std::string str = "𣧂";
    std::vector<uint16_t> actual;
    const std::tr1::array<uint16_t, 2> expect = { { 55374, 56770 } };
    c::detail::UTF8ToUTF16(str.begin(), str.end(), std::back_inserter(actual));
    EXPECT_TRUE(std::equal(expect.begin(), expect.end(), actual.begin()));
  }
}

TEST(UnicodeCase, UTF16ToUTF8) {
  {
    const std::string expect = "こんにちは";
    std::string actual;
    const std::tr1::array<uint16_t, 5> str = { { 12371, 12435, 12395, 12385, 12399 } };
    c::detail::UTF16ToUTF8(str.begin(), str.end(), std::back_inserter(actual));
    EXPECT_EQ(expect, actual);
  }
}

