#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>
#include <iv/ustring.h>
#include <iv/stringpiece.h>
#include <iv/conversions.h>
#include "test_aero.h"
namespace {

bool ExpectEqual(const iv::core::StringPiece& reg,
                 const iv::core::StringPiece& expected) {
  const iv::core::UString r = iv::core::ToUString(reg);
  iv::core::UString res;
  iv::core::RegExpEscape(reg.begin(), reg.end(), std::back_inserter(res));
  return res == iv::core::ToUString(expected);
}

}  // namespace anonymous

TEST(AeroSourceEscapeCase, SlashInBrackTest) {
  EXPECT_TRUE(ExpectEqual("[/]", "[/]")) << "[/] => [/]";
}

TEST(AeroSourceEscapeCase, SlashTest) {
  EXPECT_TRUE(ExpectEqual("/", "\\/")) << "/ => \\/";
}

TEST(AeroSourceEscapeCase, LineTerminatorTest) {
  EXPECT_TRUE(ExpectEqual("\n", "\\n")) << "\\n => \\n";
}
