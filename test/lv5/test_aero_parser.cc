#include <gtest/gtest.h>
#include "alloc.h"
#include "ustring.h"
#include "lv5/aero/parser.h"

TEST(AeroParserCase, MainTest) {
  iv::core::Space space;
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("main");
    iv::lv5::aero::Parser parser(&space, str);
    iv::lv5::aero::Expression* expr = parser.ParsePattern();
    ASSERT_TRUE(expr);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma[in]");
    iv::lv5::aero::Parser parser(&space, str);
    iv::lv5::aero::Expression* expr = parser.ParsePattern();
    ASSERT_TRUE(expr);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("(ma[in])");
    iv::lv5::aero::Parser parser(&space, str);
    iv::lv5::aero::Expression* expr = parser.ParsePattern();
    ASSERT_TRUE(expr);
  }
}

TEST(AeroParserCase, SyntaxInvalidTest) {
  iv::core::Space space;
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma[in");
    iv::lv5::aero::Parser parser(&space, str);
    iv::lv5::aero::Expression* expr = parser.ParsePattern();
    ASSERT_FALSE(expr);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma[in]]");
    iv::lv5::aero::Parser parser(&space, str);
    iv::lv5::aero::Expression* expr = parser.ParsePattern();
    ASSERT_FALSE(expr);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma([in]");
    iv::lv5::aero::Parser parser(&space, str);
    iv::lv5::aero::Expression* expr = parser.ParsePattern();
    ASSERT_FALSE(expr);
  }
}
