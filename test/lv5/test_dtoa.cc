#include <gtest/gtest.h>
#include <string>
#include "lv5/jsdtoa.h"

TEST(DToACase, FixedTest) {
  iv::lv5::StringDToA builder;
  EXPECT_EQ("0", builder.Build<iv::lv5::DTOA_FIXED>(0.2, 0, 0));
  EXPECT_EQ("0.2", builder.Build<iv::lv5::DTOA_FIXED>(0.2, 1, 0));
  EXPECT_EQ("0.20", builder.Build<iv::lv5::DTOA_FIXED>(0.2, 2, 0));
  EXPECT_EQ("0.200", builder.Build<iv::lv5::DTOA_FIXED>(0.2, 3, 0));
  EXPECT_EQ("0.2000", builder.Build<iv::lv5::DTOA_FIXED>(0.2, 4, 0));

  EXPECT_EQ("1", builder.Build<iv::lv5::DTOA_FIXED>(0.5, 0, 0));
  EXPECT_EQ("0.5", builder.Build<iv::lv5::DTOA_FIXED>(0.5, 1, 0));
  EXPECT_EQ("0.50", builder.Build<iv::lv5::DTOA_FIXED>(0.5, 2, 0));
  EXPECT_EQ("0.500", builder.Build<iv::lv5::DTOA_FIXED>(0.5, 3, 0));
  EXPECT_EQ("0.5000", builder.Build<iv::lv5::DTOA_FIXED>(0.5, 4, 0));

  EXPECT_EQ("1", builder.Build<iv::lv5::DTOA_FIXED>(1.05, 0, 0));
  EXPECT_EQ("1.1", builder.Build<iv::lv5::DTOA_FIXED>(1.05, 1, 0));
  EXPECT_EQ("1.05", builder.Build<iv::lv5::DTOA_FIXED>(1.05, 2, 0));
  EXPECT_EQ("1.050", builder.Build<iv::lv5::DTOA_FIXED>(1.05, 3, 0));
  EXPECT_EQ("1.0500", builder.Build<iv::lv5::DTOA_FIXED>(1.05, 4, 0));
}
