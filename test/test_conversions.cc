#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include "conversions.h"
#include "platform_math.h"

TEST(ConversionsCase, UStringToDoubleTest) {
  using iv::core::StringToDouble;
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("TEST", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble(" T", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble(" T ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("T ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("T", false)));
  ASSERT_EQ(0, StringToDouble(" ", false));
  ASSERT_EQ(0, StringToDouble("    ", false));
  ASSERT_EQ(0, StringToDouble("0   ", false));
  ASSERT_EQ(0, StringToDouble(" 0  ", false));
  ASSERT_EQ(0, StringToDouble("0000", false));
  ASSERT_EQ(0, StringToDouble("00  ", false));
  ASSERT_EQ(1, StringToDouble("01  ", false));
  ASSERT_EQ(8, StringToDouble("08  ", false));
  ASSERT_EQ(8, StringToDouble("  08  ", false));
  ASSERT_EQ(8, StringToDouble("  8", false));
  ASSERT_EQ(8, StringToDouble("8", false));
  ASSERT_EQ(1, StringToDouble("0x01", false));
  ASSERT_EQ(15, StringToDouble("0x0f", false));
  ASSERT_EQ(31, StringToDouble("0x1f", false));
  ASSERT_EQ(31, StringToDouble("0x1f   ", false));
  ASSERT_EQ(31, StringToDouble("    0x1f   ", false));
  ASSERT_EQ(31, StringToDouble("    0x1f", false));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0x   1f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0 x   1f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0x1 f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0 x1f  ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0X   1f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0 X   1f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    0X1 f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    00X1f", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("00X1f  ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("    00X1f  ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("00X1f", false)));
  ASSERT_EQ(100, StringToDouble("100", false));
  ASSERT_EQ(100, StringToDouble(" 100 ", false));
  ASSERT_EQ(100, StringToDouble("100   ", false));
  ASSERT_EQ(100, StringToDouble("    100", false));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("100T", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("T100", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("100     T", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("         100     T", false)));
  ASSERT_EQ(0, StringToDouble("0", false));
  ASSERT_EQ(0, StringToDouble("", false));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("E0", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("e0", false)));
  ASSERT_EQ(1, StringToDouble("1e0", false));
  ASSERT_EQ(-10, StringToDouble("-10", false));
  ASSERT_EQ(10, StringToDouble("+10", false));
  ASSERT_EQ(-10, StringToDouble(" -10 ", false));
  ASSERT_EQ(10, StringToDouble(" +10 ", false));
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble(" +Infinity ", false)));
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble(" -Infinity ", false)));
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble("+Infinity ", false)));
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble("-Infinity ", false)));
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble("  +Infinity", false)));
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble("  -Infinity", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("Infinity  ty", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("+Infinity t", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("-Infinity t", false)));
  ASSERT_GT(StringToDouble(" +Infinity ", false), 0);
  ASSERT_LT(StringToDouble(" -Infinity ", false), 0);
  ASSERT_EQ(-1.0, StringToDouble("   -1   ", false));
  ASSERT_EQ(1.0, StringToDouble("   +1   ", false));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("   -  1   ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("   +  1   ", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("+\t1", false)));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("-\t1", false)));

  ASSERT_EQ(32, StringToDouble("0x20", false));
  ASSERT_EQ(0, StringToDouble("0x20", true));

  ASSERT_EQ(1, StringToDouble("1ex", true));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("1ex", false)));

  ASSERT_EQ(0, StringToDouble("0.x", true));
  ASSERT_TRUE(iv::core::IsNaN(StringToDouble(".x", true)));

  ASSERT_TRUE(iv::core::IsNaN(StringToDouble("0.x", false)));
}

TEST(ConversionsCase, BigNumberTest) {
  using iv::core::StringToDouble;
  ASSERT_FALSE(iv::core::IsFinite(StringToDouble("1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", false)));
  ASSERT_LT(StringToDouble("1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111e-20000000000", false), 1);
  ASSERT_LT(StringToDouble("-11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111111111111111111111111111111111111111111111e-999", false), 1);
}

TEST(ConversionsCase, DoubleToStringWithRadix) {
  using iv::core::DoubleToStringWithRadix;
  ASSERT_EQ("1010", DoubleToStringWithRadix(10.0, 2));
  ASSERT_EQ("1011", DoubleToStringWithRadix(11.0, 2));
  ASSERT_EQ("22", DoubleToStringWithRadix(10.0, 4));
  ASSERT_EQ("23", DoubleToStringWithRadix(11.0, 4));
  ASSERT_EQ("12", DoubleToStringWithRadix(10.0, 8));
  ASSERT_EQ("13", DoubleToStringWithRadix(11.0, 8));
  ASSERT_EQ("b", DoubleToStringWithRadix(11.0, 16));
  ASSERT_EQ("c", DoubleToStringWithRadix(12.0, 16));
  DoubleToStringWithRadix(std::numeric_limits<double>::max(), 2);
}

TEST(ConversionsCase, StringToIntegerWithRadix) {
  using iv::core::StringToIntegerWithRadix;
  ASSERT_EQ(20, StringToIntegerWithRadix("20", 10, true));
  ASSERT_EQ(20, StringToIntegerWithRadix("20dddd", 10, true));
  ASSERT_EQ(255, StringToIntegerWithRadix("ff", 16, true));
  ASSERT_EQ(600, StringToIntegerWithRadix("go", 36, true));
  ASSERT_TRUE(iv::core::IsNaN(StringToIntegerWithRadix("20dddd", 2, true)));
}

TEST(ConversionsCase, ConvertToUInt32) {
  using iv::core::ConvertToUInt32;
  uint32_t target;
  ASSERT_FALSE(ConvertToUInt32("0x0100", &target));
  ASSERT_FALSE(ConvertToUInt32("d0100", &target));
  ASSERT_FALSE(ConvertToUInt32("20e", &target));
  ASSERT_TRUE(ConvertToUInt32("0", &target));
  ASSERT_EQ(0, target);
  ASSERT_TRUE(ConvertToUInt32("1", &target));
  ASSERT_EQ(1, target);
  ASSERT_TRUE(ConvertToUInt32("10", &target));
  ASSERT_EQ(10, target);
  ASSERT_TRUE(ConvertToUInt32("1000", &target));
  ASSERT_EQ(1000, target);
  ASSERT_FALSE(ConvertToUInt32("0100", &target));
}

TEST(ConversionsCase, BigRadix) {
  {
    const std::string str("0000001000000001001000110100010101100111100010011010101111011");
    EXPECT_EQ(18054430506169724.0, iv::core::StringToIntegerWithRadix(str, 2, false));
  }
  {
    const std::string str("123456789012345678");
    EXPECT_EQ(123456789012345680.0, iv::core::StringToIntegerWithRadix(str, 10, false));
  }
  {
    const std::string str("0x1000000000000081");
    EXPECT_EQ(1152921504606847200.0, iv::core::StringToIntegerWithRadix(str, 16, true));
  }
  {
    const std::string str("0xFFFFFFFFFFFFFB000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010");
    EXPECT_EQ(std::numeric_limits<double>::infinity(), iv::core::StringToIntegerWithRadix(str, 16, true));
  }
}
