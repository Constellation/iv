#include <gtest/gtest.h>
#include "singleton.h"
namespace {

static int global_counter_ = 0;

class A : public iv::core::Singleton<A> {
 public:
  A() : counter_(global_counter_++) { }

  int GetCounter() const {
    return counter_;
  }

  int counter_;
};

}  // namespace anonymous

TEST(SingletonCase, MainTest) {
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
  EXPECT_EQ(0, A::Instance()->GetCounter());
}
