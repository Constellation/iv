#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/assoc_vector.h>

TEST(AssocVectorCase, MainTest) {
  iv::core::AssocVector<int, int> sorted;
  sorted.insert(std::make_pair(10, 20));
  EXPECT_EQ(1, sorted.size());
  EXPECT_EQ(20, sorted[10]);
  sorted.erase(10);
  EXPECT_EQ(0, sorted.size());
  EXPECT_TRUE(sorted.end() == sorted.find(10));
  sorted[10] = 20;
  EXPECT_EQ(20, sorted[10]);
}
