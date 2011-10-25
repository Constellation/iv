#include <gtest/gtest.h>
#include "scoped_ptr.h"
#include "lv5/radio/core.h"

TEST(RadioCoreCase, MainTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
}
