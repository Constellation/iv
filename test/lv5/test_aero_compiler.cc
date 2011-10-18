#include <gtest/gtest.h>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "lv5/aero/aero.h"

TEST(AeroCompilerCase, MainTest) {
  iv::core::Space space;
  iv::lv5::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("a*?");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(data);
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("[\\u0000-\\uFFFF]");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::IGNORE_CASE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[^]]*]([^]]+])*]+");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(data);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\u10");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(data);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\u");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(data);
  }
}
