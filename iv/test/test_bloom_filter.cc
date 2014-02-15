#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/detail/cstdint.h>
#include <iv/bloom_filter.h>

TEST(BloomFilterCase, BloomFilterTest) {
  iv::core::BloomFilter<uint32_t> filter;
  filter.Add(10);
  filter.Add(20);
  EXPECT_TRUE(filter.Contains(10));
  EXPECT_TRUE(filter.Contains(20));
}

TEST(BloomFilterCase, HashedBloomFilterTest) {
  iv::core::HashedBloomFilter<std::string> filter;
  const std::string str1("TEST");
  const std::string str2("TESTING");
  filter.Add(str1);
  filter.Add(str2);
  EXPECT_TRUE(filter.Contains(str1));
  EXPECT_TRUE(filter.Contains(str2));
}
