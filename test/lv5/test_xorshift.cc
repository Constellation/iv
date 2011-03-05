#include <gtest/gtest.h>
#include <tr1/random>
#include <tr1/functional>
#include <iostream>
#include "lv5/xorshift.h"

TEST(XorshiftCase, Test) {
  iv::lv5::Xor128 g;
  for (int i = 0; i < 1000; ++i) {
    iv::lv5::Xor128::result_type res = g();
    ASSERT_LE(g.min(), res);
    ASSERT_GE(g.max(), res);
  }
}

TEST(XorshiftCase, DistIntTest) {
  typedef iv::lv5::Xor128 engine_type;
  typedef std::tr1::uniform_int<int> distribution_type;
  typedef std::tr1::variate_generator<engine_type, distribution_type> generator;
  generator gen(engine_type(), distribution_type(0, 100));
  for (int i = 0; i < 1000; ++i) {
    const int res = gen();
    ASSERT_LE(0, res);
    ASSERT_GE(100, res);
  }
}

TEST(XorshiftCase, DistRealTest) {
  typedef iv::lv5::Xor128 engine_type;
  typedef std::tr1::uniform_real<double> distribution_type;
  typedef std::tr1::variate_generator<engine_type, distribution_type> generator;
  generator gen(engine_type(), distribution_type(0, 1));
  for (int i = 0; i < 1000; ++i) {
    const double res = gen();
    ASSERT_LE(0, res);
    ASSERT_GE(1, res);
  }
}

