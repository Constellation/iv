#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>
#include <iv/ustring.h>
#include <iv/stringpiece.h>
#include <iv/conversions.h>
#include "test_aero.h"
namespace {

template<typename T>
bool ExpectEqual(T reg,
                 const iv::core::StringPiece& expected) {
  const iv::core::UString r = iv::core::ToUString(reg);
  iv::core::UString res;
  iv::core::RegExpEscape(r.begin(), r.end(), std::back_inserter(res));
  return res == iv::core::ToUString(expected);
}

}  // namespace anonymous

TEST(AeroSourceEscapeCase, SlashInBrackTest) {
  EXPECT_TRUE(ExpectEqual("[/]", "[/]")) << "[/] => [/]";
  EXPECT_TRUE(ExpectEqual("[\\]/]", "[\\]/]")) << "[\\]/] => [\\]/]";
}

TEST(AeroSourceEscapeCase, SlashTest) {
  EXPECT_TRUE(ExpectEqual("/", "\\/")) << "/ => \\/";
  EXPECT_TRUE(ExpectEqual("\\/", "\\/")) << "\\/ => \\/";
}

TEST(AeroSourceEscapeCase, LineTerminatorTest) {
  EXPECT_TRUE(ExpectEqual("\n", "\\n")) << "\\n => \\n";
  EXPECT_TRUE(ExpectEqual("\\\n", "\\\\n")) << "\\\\n => \\\\n";
  EXPECT_TRUE(ExpectEqual(0x2028, "\\u2028")) << "2028 code => \\u2028";
  EXPECT_TRUE(ExpectEqual(0x2029, "\\u2029")) << "2029 code => \\u2029";
}
