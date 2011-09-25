#include <gtest/gtest.h>
#include <string>
#include "ustring.h"
#include "string_builder.h"

TEST(StringBuilderCase, LiteralBuildTest) {
  {
    iv::core::StringBuilder builder;
    builder.Append("TEST");
    EXPECT_EQ("TEST", builder.Build());
  }
  {
    iv::core::UStringBuilder builder;
    builder.Append("TEST");
    EXPECT_TRUE(iv::core::ToUString("TEST") == builder.Build());
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
    EXPECT_TRUE(iv::core::ToUString("T") == builder.Build());
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
    const iv::core::UString str = iv::core::ToUString("T");
    builder.Append(str);
    EXPECT_TRUE(str == builder.Build());
  }
}
