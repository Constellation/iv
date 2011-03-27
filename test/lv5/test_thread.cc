#include <gtest/gtest.h>
#include "lv5/thread.h"

TEST(ThreadCase, ScopedLockTest) {
  using iv::lv5::thread::Mutex;
  using iv::lv5::thread::ScopedLock;
  Mutex mutex;
  ScopedLock<Mutex> lock(&mutex);
}
