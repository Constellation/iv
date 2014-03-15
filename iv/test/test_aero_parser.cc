#include <gtest/gtest.h>
#include <string>
#include <iv/alloc.h>
#include <iv/string.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

TEST(AeroParserCase, MainTest) {
  iv::core::Space space;
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("main");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
    iv::aero::Dumper dumper;
    EXPECT_TRUE(
        iv::core::ToU16String("DIS(ALT(main))") == dumper.Dump(data.pattern()));
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("ma[in]");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("(ma[in])");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("[\\d-a]");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String(kURLRegExp);
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{10,}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{10,20}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("\\a");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::array<char16_t, 2> l = { { 92, 99 } };
    std::u16string str(l.begin(), l.end());
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    // see IE Blog
    space.Clear();
    std::u16string str = iv::core::ToU16String("ma[in]]");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{10, 20}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{10,20");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{ 10,20}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{a10,20}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
  }
}

TEST(AeroParserCase, SyntaxInvalidTest) {
  iv::core::Space space;
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("ma[in");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("ma([in]");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("[b-a]");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("[b-aab]");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a{20,10}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("{20,10}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("+");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("*");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("{20}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("{20,}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("{0,2}");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(data.pattern());
  }
}
