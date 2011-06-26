#include <cstring>
#include <iostream>
#include <gtest/gtest.h>
#include "detail/array.h"
#include "dtoa.h"
#include "dtoa_fixed.h"
#include "dtoa_precision.h"
#include "dtoa_shortest.h"
namespace {

static void TrimRepresentation(char* buf) {
  const int len = std::strlen(buf);
  int i;
  for (i = len - 1; i >= 0; --i) {
    if (buf[i] != '0') {
      break;
    }
  }
  buf[i + 1] = '\0';
}

}  // namespace anonymous

TEST(DToACase, DToAGayFixed) {
  // dtoa(v, 3, number_digits, &decimal_point, &sign, NULL);
  std::array<char, 100> buffer;
  bool sign;
  int exponent;
  unsigned precision;
  // 6.6336520115995179509212087e-08
  for (FixedTestContainerType::const_iterator it = FixedTestContainer().begin(),
       last = FixedTestContainer().end(); it != last; ++it) {
    std::fill(buffer.begin(), buffer.end(), '\0');
    const PrecomputedFixed current_test = *it;
    double v = current_test.v;
    const int number_digits = current_test.number_digits;
    iv::core::dtoa::DoubleToASCII<false, false, true, false>(
        buffer.data(), v, number_digits, &sign, &exponent, &precision);
    EXPECT_EQ(0, sign);  // All precomputed numbers are positive.
    TrimRepresentation(buffer.data());
    EXPECT_STREQ(current_test.representation, buffer.data());
    if (exponent < 0) {
      EXPECT_EQ(current_test.decimal_point, -(-exponent - 1));
    } else {
      unsigned dec = exponent + 1;
      if (precision > dec) {
        EXPECT_EQ(current_test.decimal_point, dec);
      } else {
        // EXPECT_EQ(current_test.decimal_point, dec);
      }
    }
  }
}

TEST(DToACase, DToAGayPrecision) {
  // dtoa(v, 2, number_digits, &decimal_point, &sign, NULL);
  std::array<char, 100> buffer;
  bool sign;
  int exponent;
  unsigned precision;
  for (PrecisionTestContainerType::const_iterator it = PrecisionTestContainer().begin(),
       last = PrecisionTestContainer().end(); it != last; ++it) {
    std::fill(buffer.begin(), buffer.end(), '\0');
    const PrecomputedPrecision current_test = *it;
    double v = current_test.v;
    const int number_digits = current_test.number_digits;
    iv::core::dtoa::DoubleToASCII<false, true, false, false>(
        buffer.data(), v, number_digits, &sign, &exponent, &precision);
    EXPECT_EQ(0, sign);  // All precomputed numbers are positive.
    EXPECT_EQ(current_test.decimal_point, exponent + 1);
    EXPECT_GE(number_digits, std::strlen(buffer.data()));
    TrimRepresentation(buffer.data());
    EXPECT_STREQ(current_test.representation, buffer.data());
  }
}

TEST(DToACase, DToAGayShortest) {
  // decimal_rep = dtoa(v, 0, 0, &decimal_point, &sign, NULL);
  std::array<char, 100> buffer;
  bool sign;
  int exponent;
  unsigned precision;
  // {1.6525979369510882500000000e+15, "16525979369510882", 16},
  for (ShortestTestContainerType::const_iterator it = ShortestTestContainer().begin(),
       last = ShortestTestContainer().end(); it != last; ++it) {
    std::fill(buffer.begin(), buffer.end(), '\0');
    const PrecomputedShortest current_test = *it;
    double v = current_test.v;
    iv::core::dtoa::DoubleToASCII<true, false, false, true>(
        buffer.data(), v, 0, &sign, &exponent, &precision);
    EXPECT_EQ(0, sign);  // All precomputed numbers are positive.
    EXPECT_EQ(current_test.decimal_point, exponent + 1);
    TrimRepresentation(buffer.data());
    EXPECT_STREQ(current_test.representation, buffer.data());
  }
}

TEST(DToACase, FixedTest) {
  iv::core::dtoa::StringDToA builder;
  EXPECT_EQ("0", builder.BuildFixed(0.2, 0, 0));
  EXPECT_EQ("0.2", builder.BuildFixed(0.2, 1, 0));
  EXPECT_EQ("0.20", builder.BuildFixed(0.2, 2, 0));
  EXPECT_EQ("0.200", builder.BuildFixed(0.2, 3, 0));
  EXPECT_EQ("0.2000", builder.BuildFixed(0.2, 4, 0));

  EXPECT_EQ("1", builder.BuildFixed(0.5, 0, 0));
  EXPECT_EQ("0.5", builder.BuildFixed(0.5, 1, 0));
  EXPECT_EQ("0.50", builder.BuildFixed(0.5, 2, 0));
  EXPECT_EQ("0.500", builder.BuildFixed(0.5, 3, 0));
  EXPECT_EQ("0.5000", builder.BuildFixed(0.5, 4, 0));

  EXPECT_EQ("1", builder.BuildFixed(1.05, 0, 0));
  EXPECT_EQ("1.1", builder.BuildFixed(1.05, 1, 0));
  EXPECT_EQ("1.05", builder.BuildFixed(1.05, 2, 0));
  EXPECT_EQ("1.050", builder.BuildFixed(1.05, 3, 0));
  EXPECT_EQ("1.0500", builder.BuildFixed(1.05, 4, 0));
}
