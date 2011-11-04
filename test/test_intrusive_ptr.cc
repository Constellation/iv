#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/ref_counted.h>
#include <iv/intrusive_ptr.h>
namespace {

class Checker : public iv::core::RefCounted<Checker> {
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

static iv::core::IntrusivePtr<Checker>
MakeChecker(bool* target, std::size_t offset) {
  return iv::core::IntrusivePtr<Checker>(new Checker(target, offset), false);
}

}  // namespace anonymous

TEST(IntrusivePtrCase, IntrusivePtrTest) {
  bool target = false;
  {
    iv::core::IntrusivePtr<Checker> checker(new Checker(&target, 0), false);
  }
  EXPECT_TRUE(target);
}

TEST(IntrusivePtrCase, IntrusivePtrCopyTest) {
  bool target = false;
  {
    iv::core::IntrusivePtr<Checker> checker = MakeChecker(&target, 0);
    EXPECT_FALSE(target);
  }
  EXPECT_TRUE(target);
}

TEST(IntrusivePtrCase, IntrusivePtrCopyScopedTest) {
  bool target = false;
  {
    iv::core::IntrusivePtr<Checker> upper;
    {
      iv::core::IntrusivePtr<Checker> checker = MakeChecker(&target, 0);
      EXPECT_FALSE(target);
      upper = checker;
    }
    EXPECT_FALSE(target);
  }
  EXPECT_TRUE(target);
}
