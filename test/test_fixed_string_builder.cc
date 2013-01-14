#include <algorithm>
#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/fixed_string_builder.h>

TEST(FixedStringBuilderCase, MainTest) {
  typedef std::array<char, 100> array_type;
  array_type buffer;
  {
    std::fill<array_type::iterator, char>(buffer.begin(), buffer.end(), '\0');
    iv::core::FixedStringBuilder builder(buffer.data(), buffer.size());
    builder.AddCharacter('T');
    builder.AddCharacter('H');
    builder.AddCharacter('I');
    builder.AddCharacter('S');
    builder.AddString(" IS ");
    builder.AddString("TEST");
    EXPECT_STREQ("THIS IS TEST", builder.Finalize());
  }
  {
    std::fill<array_type::iterator, char>(buffer.begin(), buffer.end(), '\0');
    iv::core::FixedStringBuilder builder(buffer.data(), buffer.size());
    builder.AddPading('T', 20);
    EXPECT_STREQ("TTTTTTTTTTTTTTTTTTTT", builder.Finalize());
  }
  {
    std::fill<array_type::iterator, char>(buffer.begin(), buffer.end(), '\0');
    iv::core::FixedStringBuilder builder(buffer.data(), buffer.size());
    builder.AddInteger(20000);
    EXPECT_STREQ("20000", builder.Finalize());
  }
  {
    std::fill<array_type::iterator, char>(buffer.begin(), buffer.end(), '\0');
    iv::core::FixedStringBuilder builder(buffer.data(), buffer.size());
    builder.AddSubstring("TESTING", 4);
    EXPECT_STREQ("TEST", builder.Finalize());
  }
}
