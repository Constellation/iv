#include <gtest/gtest.h>
#include <string>
#include "lv5/jsdtoa.h"

TEST(DToACase, FixedTest) {
  iv::lv5::StringDToA builder;
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.2, 0, 0), "0");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.2, 1, 0), "0.2");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.2, 2, 0), "0.20");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.2, 3, 0), "0.200");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.2, 4, 0), "0.2000");

  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.5, 0, 0), "1");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.5, 1, 0), "0.5");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.5, 2, 0), "0.50");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.5, 3, 0), "0.500");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(0.5, 4, 0), "0.5000");

  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(1.05, 0, 0), "1");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(1.05, 1, 0), "1.1");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(1.05, 2, 0), "1.05");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(1.05, 3, 0), "1.050");
  EXPECT_EQ(builder.Build<iv::lv5::DTOA_FIXED>(1.05, 4, 0), "1.0500");
}
