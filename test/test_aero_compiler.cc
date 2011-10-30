#include <gtest/gtest.h>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "scoped_ptr.h"
#include "aero/aero.h"

TEST(AeroCompilerCase, MainTest) {
  iv::core::Space space;
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("a*?");
    iv::aero::Parser parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("t+");
    iv::aero::Parser parser(&space, str, iv::aero::IGNORE_CASE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("[\\u0000-\\uFFFF]");
    iv::aero::Parser parser(&space, str, iv::aero::IGNORE_CASE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[^]]*]([^]]+])*]+");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\u10");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\u");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
  }
}
