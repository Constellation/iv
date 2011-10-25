#include <gtest/gtest.h>
#include <ctime>
#include "mt19937.h"
#include "random.h"

TEST(MT19937Case, DistRealTest) {
  typedef iv::core::MT19937 engine_type;
  typedef iv::core::UniformRandomGenerator<engine_type> generator;
  generator gen(0.0, 1.0, std::time(NULL));
  for (int i = 0; i < 10000000; ++i) {
    const double res = gen.get();
    ASSERT_LE(0.0, res);
    ASSERT_GT(1.0, res);
  }
}
