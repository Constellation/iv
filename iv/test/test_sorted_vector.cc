#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/sorted_vector.h>

TEST(SortedVectorCase, SortedTest) {
  iv::core::SortedVector<int> sorted;
  sorted.push_back(10);
  sorted.push_back(1);
  sorted.push_back(2);
  std::size_t index = 0;
  std::array<int, 3> expected = { {
    1, 2, 10
  } };
  for (iv::core::SortedVector<int>::const_iterator it = sorted.begin(),
       last = sorted.end(); it != last; ++it, ++index) {
    EXPECT_EQ(expected[index], *it);
  }
}
