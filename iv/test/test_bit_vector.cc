#include <gtest/gtest.h>
#include <iv/bit_vector.h>

TEST(BitVectorCase, MainTest) {
  iv::core::BitVector<> vec;
  vec.resize(10, false);
  EXPECT_TRUE(vec.none());
  EXPECT_FALSE(vec.any());
  EXPECT_EQ(10u, vec.size());
  for (std::size_t i = 0; i < 10u; ++i) {
    EXPECT_FALSE(vec.test(i));
  }
  vec.set(0);
  EXPECT_TRUE(vec[0]);
  vec.resize(20, false);
  EXPECT_TRUE(vec[0]);
  EXPECT_TRUE(vec.any());
  EXPECT_FALSE(vec.none());
  for (std::size_t i = 1; i < 20u; ++i) {
    EXPECT_FALSE(vec.test(i));
  }
}

TEST(BitVectorCase, FlipTest) {
  iv::core::BitVector<> vec(10, false);
  const iv::core::BitVector<> vec2 = ~vec;
  for (std::size_t i = 0; i < 10u; ++i) {
    EXPECT_FALSE(vec.test(i));
    EXPECT_TRUE(vec2.test(i));
  }
  EXPECT_TRUE(vec != vec2);
  EXPECT_FALSE(vec == vec2);
  vec.flip();
  for (std::size_t i = 0; i < 10u; ++i) {
    EXPECT_TRUE(vec.test(i));
  }
  EXPECT_FALSE(vec != vec2);
  EXPECT_TRUE(vec == vec2);
}

TEST(BitVectorCase, EqualityTest) {
  for (std::size_t i = 1; i < 129u; ++i) {
    iv::core::BitVector<> vec(i, false);
    iv::core::BitVector<> vec2 = vec;
    EXPECT_TRUE(vec == vec2) << i;
    EXPECT_FALSE(vec != vec2) << i;
    vec.flip();
    EXPECT_TRUE(vec != vec2) << i;
    EXPECT_FALSE(vec == vec2) << i;
  }

  iv::core::BitVector<> vec1(0, false);
  iv::core::BitVector<> vec2(0, false);
  EXPECT_TRUE(vec1 == vec2);
  EXPECT_TRUE(vec1.flip() == vec2);
}
