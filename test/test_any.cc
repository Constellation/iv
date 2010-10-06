#include <gtest/gtest.h>
#include <string>
#include "any.h"

TEST(AnyCase, AnyTest) {
  using iv::core::Any;
  Any i1 = 20;
  Any i2 = &i1;
  const char * test = "TEST";
  std::string str(test);
  Any i3 = test;
  Any i4 = "TEST";
  i3 = i4;
  i4 = str;
}

TEST(AnyCase, VectorAnyTest) {
  using iv::core::Any;
  std::vector<Any> vec;

  vec.push_back(10);
  vec.push_back(std::string("TEST"));

  std::string test_val("TEST");
  Any val1 = vec[0];
  int val2 = *(val1.As<int>());
  val1 = vec[1];
  std::string val3 = *(val1.As<std::string>());
  EXPECT_EQ(val2, 10);
  EXPECT_EQ(val3, test_val);
}
