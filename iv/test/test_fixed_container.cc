#include <gtest/gtest.h>
#include <iv/fixed_container.h>

TEST(FixedContainerCase, MainTest) {
  using iv::core::FixedContainer;
  char buf[100];
  FixedContainer<char> container(buf, 100);
  container[0] = 'a';
  EXPECT_EQ('a', buf[0]);
}
