#include <gtest/gtest.h>
#include "detail/array.h"
#include "scoped_ptr.h"
namespace {

class Checker {
 public:
  Checker(bool* target, std::size_t offset) : target_(target), offset_(offset) { }

  Checker() : target_(NULL), offset_(0) { }

  ~Checker() {
    if (target_) {
      target_[offset_] = true;
    }
  }

  void StoreTargetAndOffset(bool* target, std::size_t offset) {
    target_ = target;
    offset_ = offset;
  }

 private:
  bool* target_;
  std::size_t offset_;
};

}  // namespace anonymous

TEST(ScopedPtrCase, ScopedPtrTest) {
  bool target = false;
  {
    iv::core::ScopedPtr<Checker> ptr(new Checker(&target, 0));
  }
  EXPECT_TRUE(target);
}

TEST(ScopedPtrCase, ScopedPtrBoolTest) {
  {
    iv::core::ScopedPtr<int> ptr(new int(10));
    EXPECT_TRUE(ptr);
  }
  {
    iv::core::ScopedPtr<Checker> ptr;
    EXPECT_TRUE(NULL == ptr.get());
    EXPECT_FALSE(ptr);
  }
}

TEST(ScopedPtrCase, ScopedArrayTest) {
  std::array<bool, 10> results;
  {
    iv::core::ScopedPtr<Checker[]> ary(new Checker[10]);
    for (std::size_t i = 0; i < 10; ++i) {
      ary[i].StoreTargetAndOffset(results.data(), i);
    }
  }
  for (std::array<bool, 10>::const_iterator it = results.begin(),
       last = results.end(); it != last; ++it) {
    EXPECT_TRUE(*it);
  }
}
