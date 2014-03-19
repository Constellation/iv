#include <gtest/gtest.h>
#include <iv/small_vector.h>

using namespace iv;

TEST(SmallVectorCase, SizeTest) {
  core::small_vector<int, 20> vec;
  EXPECT_EQ(0u, vec.size());
}

TEST(SmallVectorCase, ReserveTest) {
  core::small_vector<int, 10> vec;
  vec.reserve(100u);
  EXPECT_LE(100u, vec.capacity());
}

TEST(SmallVectorCase, PushBackTest) {
  core::small_vector<int, 10> vec;
  vec.push_back(2000);
  EXPECT_LE(1u, vec.size());
  int value = 42;
  vec.push_back(value);
  EXPECT_LE(2u, vec.size());
  EXPECT_EQ(2000, vec[0]);
  EXPECT_EQ(42, vec[1]);
}

class DeallocCheck {
 public:
  DeallocCheck(bool* value)
    : value_(value) {
  }
  ~DeallocCheck() {
    *value_ = true;
  }
 private:
  bool* value_;
};

TEST(SmallVectorCase, DeallocTest) {
  bool value = false;
  {
    core::small_vector<DeallocCheck, 10> vec;
    vec.emplace_back(&value);
  }
  EXPECT_TRUE(value);
}

TEST(SmallVectorCase, PopBackTest) {
    core::small_vector<int, 10> vec;
    vec.push_back(20);
    vec.pop_back();
    EXPECT_EQ(0u, vec.size());
}

TEST(SmallVectorCase, IteratorTest) {
  core::small_vector<int, 20> vec;
  EXPECT_EQ(0u, vec.size());
}

TEST(SmallVectorCase, AllocatorTest) {
  core::small_vector<int, 20> vec;
  std::allocator<int> alloc = vec.get_allocator();
}
