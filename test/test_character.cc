#include <gtest/gtest.h>
#include "character.h"

TEST(CharacterCase, CategoryTest) {
  using iv::core::character::GetCategory;
  for (uint16_t i = 0; i < 0xffff; ++i) {
    GetCategory(i);
  }
  ASSERT_EQ(0, GetCategory(0x037F));
}

TEST(CharacterCase, ToUpperCaseTest) {
  using iv::core::character::ToUpperCase;
  // for ascii test
  for (uint16_t ch = 'a',
       target = 'A'; ch <= 'z'; ++ch, ++target) {
    ASSERT_EQ(ToUpperCase(ch), target);
  }
  ASSERT_EQ(0x00CB, ToUpperCase(0x00EB));
  ASSERT_EQ(0x0531, ToUpperCase(0x0561));
  ASSERT_EQ('A', ToUpperCase('A'));
  ASSERT_EQ(0x0560, ToUpperCase(0x0560));
}

TEST(CharacterCase, ToLowerCaseTest) {
  using iv::core::character::ToLowerCase;
  // for ascii test
  for (uint16_t ch = 'A',
       target = 'a'; ch <= 'A'; ++ch, ++target) {
    ASSERT_EQ(ToLowerCase(ch), target);
  }
  ASSERT_EQ(0x0561, ToLowerCase(0x0531));
  ASSERT_EQ(0x0560, ToLowerCase(0x0560));
}
