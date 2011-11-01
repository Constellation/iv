#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <iv/digit_iterator.h>

using iv::core::DigitIterator;

TEST(DigitIteratorCase, DigitIteratorTest) {
  typedef DigitIterator<std::string::const_iterator> Iterator;
  {
    const std::string str("FF");
    for (Iterator it = Iterator(16, str.begin(), str.end()),
         last = Iterator(); it != last; ++it) {
      EXPECT_EQ(1, *it);
    }
  }

  {
    const std::string str("10101");
    std::string::const_iterator s = str.begin();
    for (Iterator it = Iterator(2, str.begin(), str.end()),
         last = Iterator(); it != last; ++it, ++s) {
      EXPECT_EQ(*s == '1', *it);
    }
  }

  {
    const std::string str("111111111111111111111111111111111111111111111111110000000000000100000000000110110111101111111111111111111111111111111111111");
    std::string::const_iterator s = str.begin();
    for (Iterator it = Iterator(2, str.begin(), str.end()),
         last = Iterator(); it != last; ++it, ++s) {
      EXPECT_EQ(*s == '1', *it);
    }
  }

  {
    const std::string str("1111101");
    std::string::const_iterator s = str.begin();
    for (Iterator it = Iterator(2, str.begin(), str.end()),
         last = Iterator(); it != last; ++it, ++s) {
      EXPECT_EQ(*s == '1', *it);
    }
  }
}
