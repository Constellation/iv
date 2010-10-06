#include <gtest/gtest.h>
#include <string>
#include <ctime>
#include "mt19937.h"

TEST(MT19937Case, IntTest) {
  iv::core::MT19937 engine(std::time(NULL));
  iv::core::UniformIntDistribution<int> dist(0, 100);
  for (int i = 0; i < 1000; ++i) {
    int val = dist(engine);
    EXPECT_TRUE(0 <= val && val <= 100);
  }
}

TEST(MT19937Case, DoubleTest) {
  iv::core::MT19937 engine(std::time(NULL));
  iv::core::UniformIntDistribution<int> dist(0, 100);
  for (int i = 0; i < 1000; ++i) {
    double val = dist(engine);
    EXPECT_TRUE(0 <= val && val <= 100);
  }
}
