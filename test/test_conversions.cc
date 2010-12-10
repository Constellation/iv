#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <cmath>
#include "conversions.h"

TEST(ConversionsCase, UStringToDoubleTest) {
  using iv::core::StringToDouble;
  ASSERT_TRUE(std::isnan(StringToDouble("TEST", false)));
  ASSERT_TRUE(std::isnan(StringToDouble(" T", false)));
  ASSERT_TRUE(std::isnan(StringToDouble(" T ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("T ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("T", false)));
  ASSERT_EQ(StringToDouble(" ", false), 0);
  ASSERT_EQ(StringToDouble("    ", false), 0);
  ASSERT_EQ(StringToDouble("0   ", false), 0);
  ASSERT_EQ(StringToDouble(" 0  ", false), 0);
  ASSERT_EQ(StringToDouble("0000", false), 0);
  ASSERT_EQ(StringToDouble("00  ", false), 0);
  ASSERT_EQ(StringToDouble("01  ", false), 1);
  ASSERT_EQ(StringToDouble("08  ", false), 8);
  ASSERT_EQ(StringToDouble("  08  ", false), 8);
  ASSERT_EQ(StringToDouble("  8", false), 8);
  ASSERT_EQ(StringToDouble("8", false), 8);
  ASSERT_EQ(StringToDouble("0x01", false), 1);
  ASSERT_EQ(StringToDouble("0x0f", false), 15);
  ASSERT_EQ(StringToDouble("0x1f", false), 31);
  ASSERT_EQ(StringToDouble("0x1f   ", false), 31);
  ASSERT_EQ(StringToDouble("    0x1f   ", false), 31);
  ASSERT_EQ(StringToDouble("    0x1f", false), 31);
  ASSERT_TRUE(std::isnan(StringToDouble("    0x   1f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    0 x   1f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    0x1 f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    0 x1f  ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    0X   1f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    0 X   1f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    0X1 f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    00X1f", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("00X1f  ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("    00X1f  ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("00X1f", false)));
  ASSERT_EQ(StringToDouble("100", false), 100);
  ASSERT_EQ(StringToDouble(" 100 ", false), 100);
  ASSERT_EQ(StringToDouble("100   ", false), 100);
  ASSERT_EQ(StringToDouble("    100", false), 100);
  ASSERT_TRUE(std::isnan(StringToDouble("100T", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("T100", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("100     T", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("         100     T", false)));
  ASSERT_EQ(StringToDouble("0", false), 0);
  ASSERT_EQ(StringToDouble("", false), 0);
  ASSERT_TRUE(std::isnan(StringToDouble("E0", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("e0", false)));
  ASSERT_EQ(StringToDouble("1e0", false), 1);
  ASSERT_EQ(StringToDouble("-10", false), -10);
  ASSERT_EQ(StringToDouble("+10", false), 10);
  ASSERT_EQ(StringToDouble(" -10 ", false), -10);
  ASSERT_EQ(StringToDouble(" +10 ", false), 10);
  ASSERT_FALSE(std::isfinite(StringToDouble(" +Infinity ", false)));
  ASSERT_FALSE(std::isfinite(StringToDouble(" -Infinity ", false)));
  ASSERT_FALSE(std::isfinite(StringToDouble("+Infinity ", false)));
  ASSERT_FALSE(std::isfinite(StringToDouble("-Infinity ", false)));
  ASSERT_FALSE(std::isfinite(StringToDouble("  +Infinity", false)));
  ASSERT_FALSE(std::isfinite(StringToDouble("  -Infinity", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("Infinity  ty", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("+Infinity t", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("-Infinity t", false)));
  ASSERT_GT(StringToDouble(" +Infinity ", false), 0);
  ASSERT_LT(StringToDouble(" -Infinity ", false), 0);
  ASSERT_EQ(-1.0, StringToDouble("   -1   ", false));
  ASSERT_EQ(1.0, StringToDouble("   +1   ", false));
  ASSERT_TRUE(std::isnan(StringToDouble("   -  1   ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("   +  1   ", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("+\t1", false)));
  ASSERT_TRUE(std::isnan(StringToDouble("-\t1", false)));

  ASSERT_EQ(StringToDouble("0x20", false), 32);
  ASSERT_EQ(StringToDouble("0x20", true), 0);

  ASSERT_EQ(StringToDouble("1ex", true), 1);
  ASSERT_TRUE(std::isnan(StringToDouble("1ex", false)));

  ASSERT_EQ(StringToDouble("0.x", true), 0);
  ASSERT_TRUE(std::isnan(StringToDouble(".x", true)));

  ASSERT_TRUE(std::isnan(StringToDouble("0.x", false)));
}

TEST(ConversionsCase, BigNumberTest) {
  using iv::core::StringToDouble;
  ASSERT_FALSE(std::isfinite(StringToDouble("1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", false)));
  ASSERT_LT(StringToDouble("1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111e-20000000000", false), 1);
  ASSERT_LT(StringToDouble("-11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111111111111111111111111111111111111111111111e-999", false), 1);
}

TEST(ConversionsCase, DoubleToStringWithRadix) {
  using iv::core::DoubleToStringWithRadix;
  ASSERT_EQ(DoubleToStringWithRadix(10.0, 2), "1010");
  ASSERT_EQ(DoubleToStringWithRadix(11.0, 2), "1011");
  ASSERT_EQ(DoubleToStringWithRadix(10.0, 4), "22");
  ASSERT_EQ(DoubleToStringWithRadix(11.0, 4), "23");
  ASSERT_EQ(DoubleToStringWithRadix(10.0, 8), "12");
  ASSERT_EQ(DoubleToStringWithRadix(11.0, 8), "13");
  ASSERT_EQ(DoubleToStringWithRadix(11.0, 16), "b");
  ASSERT_EQ(DoubleToStringWithRadix(12.0, 16), "c");
  DoubleToStringWithRadix(std::numeric_limits<double>::max(), 2);
}

TEST(ConversionsCase, StringToIntegerWithRadix) {
  using iv::core::StringToIntegerWithRadix;
  ASSERT_EQ(StringToIntegerWithRadix("20", 10, true), 20);
  ASSERT_EQ(StringToIntegerWithRadix("20dddd", 10, true), 20);
  ASSERT_EQ(StringToIntegerWithRadix("ff", 16, true), 255);
  ASSERT_EQ(StringToIntegerWithRadix("go", 36, true), 600);
  ASSERT_TRUE(std::isnan(StringToIntegerWithRadix("20dddd", 2, true)));
}

TEST(ConversionsCase, ConvertToUInt32) {
  using iv::core::ConvertToUInt32;
  uint32_t target;
  ASSERT_FALSE(ConvertToUInt32("0x0100", &target));
  ASSERT_FALSE(ConvertToUInt32("d0100", &target));
  ASSERT_FALSE(ConvertToUInt32("20e", &target));
  ASSERT_TRUE(ConvertToUInt32("0", &target));
  ASSERT_EQ(target, 0);
  ASSERT_TRUE(ConvertToUInt32("1", &target));
  ASSERT_EQ(target, 1);
  ASSERT_TRUE(ConvertToUInt32("10", &target));
  ASSERT_EQ(target, 10);
  ASSERT_TRUE(ConvertToUInt32("1000", &target));
  ASSERT_EQ(target, 1000);
  ASSERT_FALSE(ConvertToUInt32("0100", &target));
}
