#include <string>
#include <iterator>
#include <algorithm>
#include <gtest/gtest.h>
#include <iv/detail/cstdint.h>
#include <iv/unicode.h>

namespace c = iv::core::unicode;

TEST(UnicodeCase, MaskTest) {
  const uint32_t ch = 0xFFFF;
  EXPECT_EQ(0xFu, c::Mask<4>(ch));
  EXPECT_EQ(0xFFu, c::Mask<8>(ch));
  EXPECT_EQ(0xFFFu, c::Mask<12>(ch));
  EXPECT_EQ(0xFFFFu, c::Mask<16>(ch));
}

TEST(UnicodeCase, UTF8ToUCS4) {
  const uint8_t konnichiha[] = {
    0xe3, 0x81, 0x93, 0xe3, 0x82, 0x93, 0xe3, 0x81, 0xab, 0xe3, 0x81, 0xa1, 0xe3, 0x81, 0xaf
  };
  std::vector<uint32_t> actual;
  const std::array<uint32_t, 5> expect = { { 12371, 12435, 12395, 12385, 12399 } };
  EXPECT_EQ(c::UNICODE_NO_ERROR,
            c::UTF8ToUCS4(konnichiha, konnichiha + sizeof(konnichiha) / sizeof(uint8_t), std::back_inserter(actual)));
  EXPECT_TRUE(std::equal(expect.begin(), expect.end(), actual.begin()));
}

TEST(UnicodeCase, UTF8ToUTF16) {
  {
    const uint8_t konnichiha[] = {
      0xe3, 0x81, 0x93, 0xe3, 0x82, 0x93, 0xe3, 0x81, 0xab, 0xe3, 0x81, 0xa1, 0xe3, 0x81, 0xaf
    };
    std::vector<char16_t> actual;
    const std::array<char16_t, 5> expect = { { 12371, 12435, 12395, 12385, 12399 } };
    EXPECT_EQ(c::UNICODE_NO_ERROR,
              c::UTF8ToUTF16(konnichiha, konnichiha + sizeof(konnichiha) / sizeof(uint8_t), std::back_inserter(actual)));
    EXPECT_TRUE(std::equal(expect.begin(), expect.end(), actual.begin()));
  }
  {
    // surrogate pair test
    const uint8_t str[] = {
      0xf0, 0xa3, 0xa7, 0x82, 0x20
    };
    std::vector<char16_t> actual;
    const std::array<char16_t, 2> expect = { { 55374, 56770 } };
    EXPECT_EQ(c::UNICODE_NO_ERROR, c::UTF8ToUTF16(str, str + sizeof(str) / sizeof(uint8_t), std::back_inserter(actual)));
    EXPECT_TRUE(std::equal(expect.begin(), expect.end(), actual.begin()));
  }
  {
    // surrogate area test
    const std::array<uint8_t, 3> str = { { 0xED, 0xA0, 0x80 } };
    std::vector<char16_t> actual;
    EXPECT_EQ(c::INVALID_SEQUENCE, c::UTF8ToUTF16(str.begin(), str.end(), std::back_inserter(actual)));
  }
}

TEST(UnicodeCase, UTF16ToUTF8) {
  {
    const uint8_t konnichiha[] = {
      0xe3, 0x81, 0x93, 0xe3, 0x82, 0x93, 0xe3, 0x81, 0xab, 0xe3, 0x81, 0xa1, 0xe3, 0x81, 0xaf
    };
    const std::string expect(konnichiha, konnichiha + sizeof(konnichiha) / sizeof(uint8_t));
    std::string actual;
    const std::array<char16_t, 5> str = { { 12371, 12435, 12395, 12385, 12399 } };
    EXPECT_EQ(c::UNICODE_NO_ERROR, c::UTF16ToUTF8(str.begin(), str.end(), std::back_inserter(actual)));
    EXPECT_EQ(expect, actual);
  }
  {
    // surrogate pair test
    const uint8_t target[] = {
      0xf0, 0xa3, 0xa7, 0x82
    };
    const std::string expect(target, target + sizeof(target) / sizeof(uint8_t));
    std::string actual;
    const std::array<char16_t, 2> str = { { 55374, 56770 } };
    EXPECT_EQ(c::UNICODE_NO_ERROR, c::UTF16ToUTF8(str.begin(), str.end(), std::back_inserter(actual)));
    EXPECT_EQ(expect, actual);
  }
  {
    // surrogate pair test
    // invalid sequence, should be fail
    std::string actual;
    const std::array<char16_t, 2> str = { { 56770, 55374 } };
    EXPECT_EQ(c::INVALID_SEQUENCE, c::UTF16ToUTF8(str.begin(), str.end(), std::back_inserter(actual)));
  }
}
