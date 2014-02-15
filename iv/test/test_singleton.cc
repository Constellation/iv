#include <gtest/gtest.h>
#include <iv/singleton.h>
namespace {

static int global_counter_ = 0;

class A : public iv::core::Singleton<A> {
 private:
  friend class iv::core::Singleton<A>;
  A() : counter_(global_counter_++) { }
  ~A() { }

 public:
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

TEST(SingletonCase, FailTest) {
  // compile error
  // A* a = new A;
  // delete a;
  // compile error
  // delete A::Instance();
}
