#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/string_view.h>
#include <iv/conversions.h>
#include <iv/aero/aero.h>
#include <iv/ignore_unused_variable_warning.h>
#include "test_aero.h"
namespace {

template<typename T>
bool ExpectEqual(T reg,
                 const iv::core::string_view& expected) {
  const iv::core::UString r = iv::core::ToUString(reg);
  iv::core::UString res;
  iv::core::RegExpEscape(r.begin(), r.end(), std::back_inserter(res));
  return res == iv::core::ToUString(expected);
}

}  // namespace anonymous

TEST(AeroSourceEscapeCase, SlashInBrackTest) {
  EXPECT_TRUE(ExpectEqual("[/]", "[/]")) << "[/] => [/]";
  EXPECT_TRUE(ExpectEqual("[\\]/]", "[\\]/]")) << "[\\]/] => [\\]/]";
  EXPECT_TRUE(ExpectEqual("\\\\[/]", "\\\\[/]")) << "\\\\[/] => \\\\[/]";
}

TEST(AeroSourceEscapeCase, SlashTest) {
  EXPECT_TRUE(ExpectEqual("/", "\\/")) << "/ => \\/";
  EXPECT_TRUE(ExpectEqual("\\/", "\\/")) << "\\/ => \\/";
}

TEST(AeroSourceEscapeCase, LineTerminatorTest) {
  EXPECT_TRUE(ExpectEqual("\n", "\\n")) << "\\n => \\n";
  EXPECT_TRUE(ExpectEqual("\\\n", "\\n")) << "\\\\n code => \\n";
  EXPECT_TRUE(ExpectEqual("\r", "\\r")) << "\\r => \\r";
  EXPECT_TRUE(ExpectEqual("\\\r", "\\r")) << "\\\\r code => \\r";
  EXPECT_TRUE(ExpectEqual(0x2028, "\\u2028")) << "2028 code => \\u2028";
  {
    iv::core::UString str;
    str.push_back('\\');
    str.push_back(0x2028);
    EXPECT_TRUE(ExpectEqual(str, "\\u2028")) << "\\2028 code => \\u2028";
  }
  EXPECT_TRUE(ExpectEqual(0x2029, "\\u2029")) << "2029 code => \\u2029";
  {
    iv::core::UString str;
    str.push_back('\\');
    str.push_back(0x2029);
    EXPECT_TRUE(ExpectEqual(str, "\\u2029")) << "\\2029 code => \\u2029";
  }
}

TEST(AeroSourceEscapeCase, LineTerminatorInBrackTest) {
  EXPECT_TRUE(ExpectEqual("[\n]", "[\\n]")) << "[\\n] => [\\n]";
  EXPECT_TRUE(ExpectEqual("[\\\n]", "[\\n]")) << "[\\\\n code] => [\\n]";
  EXPECT_TRUE(ExpectEqual("[\r]", "[\\r]")) << "[\\r] => [\\r]";
  EXPECT_TRUE(ExpectEqual("[\\\r]", "[\\r]")) << "[\\\\r code] => [\\r]";
  {
    iv::core::UString str;
    str.push_back('[');
    str.push_back(0x2028);
    str.push_back(']');
    EXPECT_TRUE(ExpectEqual(str, "[\\u2028]")) << "[\\2028 code] => [\\u2028]";
  }
  {
    iv::core::UString str;
    str.push_back('[');
    str.push_back('\\');
    str.push_back(0x2028);
    str.push_back(']');
    EXPECT_TRUE(ExpectEqual(str, "[\\u2028]")) << "[\\\\2028 code] => [\\u2028]";
  }
  {
    iv::core::UString str;
    str.push_back('[');
    str.push_back(0x2029);
    str.push_back(']');
    EXPECT_TRUE(ExpectEqual(str, "[\\u2029]")) << "[\\2029 code] => [\\u2029]";
  }
  {
    iv::core::UString str;
    str.push_back('[');
    str.push_back('\\');
    str.push_back(0x2029);
    str.push_back(']');
    EXPECT_TRUE(ExpectEqual(str, "[\\u2029]")) << "[\\\\2029 code] => [\\u2029]";
  }
}

TEST(AeroSourceEscapeCase, SpecialCharactersTest) {
  EXPECT_TRUE(ExpectEqual("^$\\.*+?()[]{}|!=", "^$\\.*+?()[]{}|!="));
}

TEST(AeroSourceEscapeCase, OneCharTest) {
  iv::core::Space space;
  for (uint32_t ch = 0; ch <= 0xFFFF; ++ch) {
    iv::core::UString str = iv::core::ToUString(ch);
    {
      space.Clear();
      iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
      int error = 0;
      parser.ParsePattern(&error);
      if (error) {  // invalid, like '['
        continue;
      }
    }
    iv::core::UString res;
    iv::core::RegExpEscape(str.begin(), str.end(), std::back_inserter(res));
    {
      space.Clear();
      iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
      int error = 0;
      iv::aero::ParsedData data = parser.ParsePattern(&error);
      EXPECT_FALSE(error);
      iv::core::ignore_unused_variable_warning(data);
    }
  }
}
