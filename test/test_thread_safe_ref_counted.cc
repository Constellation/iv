#include <gtest/gtest.h>
#include <iv/thread_safe_ref_counted.h>
namespace {

class Check : public iv::core::ThreadSafeRefCounted<Check> {
 public:
  Check(bool* target) : target_(target) { }
  ~Check() {
    *target_ = true;
  }
 private:
  bool* target_;
};

}  // namespace anonymous

TEST(ThreadSafeRefCountedCase, ThreadSafeRefCountedTest) {
  bool target = false;
  Check* check = new Check(&target);
  EXPECT_EQ(check->RetainCount(), 1);
  check->Release();
  EXPECT_TRUE(target);
}
