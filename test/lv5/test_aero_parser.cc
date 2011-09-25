#include <gtest/gtest.h>
#include "test_aero.h"
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "lv5/aero/parser.h"
#include "lv5/aero/flags.h"
#include "lv5/aero/dumper.h"

TEST(AeroParserCase, MainTest) {
  iv::core::Space space;
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("main");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(dis);
    iv::lv5::aero::Dumper dumper;
    EXPECT_TRUE(iv::core::ToUString("DIS(ALT(main))") == dumper.Dump(dis));
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma[in]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("(ma[in])");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("[\\d-a]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString(kURLRegExp);
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(dis);
  }
}

TEST(AeroParserCase, SyntaxInvalidTest) {
  iv::core::Space space;
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma[in");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma[in]]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("ma([in]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("[b-a]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(dis);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("[b-aab]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_TRUE(error);
    ASSERT_FALSE(dis);
  }
}
