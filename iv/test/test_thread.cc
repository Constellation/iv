#include <gtest/gtest.h>
#include <iv/thread.h>

TEST(ThreadCase, ScopedLockTest) {
  using iv::core::thread::Mutex;
  using iv::core::thread::ScopedLock;
  Mutex mutex;
  ScopedLock<Mutex> lock(&mutex);
}
