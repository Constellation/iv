#include <gtest/gtest.h>
#include <string>
#include <iv/ustring.h>
#include <iv/string_builder.h>

TEST(StringBuilderCase, LiteralBuildTest) {
  {
    iv::core::StringBuilder builder;
    builder.Append("TEST");
    EXPECT_EQ("TEST", builder.Build());
  }
  {
    iv::core::UStringBuilder builder;
    builder.Append("TEST");
    EXPECT_TRUE(iv::core::ToU16String("TEST") == builder.Build());
  }
}

TEST(StringBuilderCase, CharBuildTest) {
  {
    iv::core::StringBuilder builder;
    builder.Append('T');
    EXPECT_EQ("T", builder.Build());
  }
  {
    iv::core::UStringBuilder builder;
    builder.Append('T');
    EXPECT_TRUE(iv::core::ToU16String("T") == builder.Build());
  }
}

TEST(StringBuilderCase, StringBuildTest) {
  {
    iv::core::StringBuilder builder;
    const std::string str("T");
    builder.Append(str);
    EXPECT_EQ(str, builder.Build());
  }
  {
    iv::core::UStringBuilder builder;
    const std::u16string str = iv::core::ToU16String("T");
    builder.Append(str);
    EXPECT_TRUE(str == builder.Build());
  }
}
