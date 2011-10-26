#include <gtest/gtest.h>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "scoped_ptr.h"
#include "aero/aero.h"
#include "test_aero.h"

TEST(AeroFilterCase, ContentTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("s$");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 's');
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^s");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 0);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\d");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 0);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\s");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 0);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(s|t)");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 's' | 't');
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?:s|t)");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 's' | 't');
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?:s|\\d)");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 0);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?=s)|d");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 0);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?!s)|d");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_EQ(code->filter(), 0);
  }
}
