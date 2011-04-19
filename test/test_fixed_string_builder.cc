#include <tr1/array>
#include <gtest/gtest.h>
#include "fixed_string_builder.h"

TEST(FixedStringBuilderCase, MainTest) {
  std::tr1::array<char, 100> buffer;
  iv::core::FixedStringBuilder builder(buffer.data(), buffer.size());
  builder.AddCharacter('T');
  builder.AddCharacter('H');
  builder.AddCharacter('I');
  builder.AddCharacter('S');
  builder.AddString(" IS ");
  builder.AddString("TEST");
  EXPECT_STREQ("THIS IS TEST", builder.Finalize());
}
