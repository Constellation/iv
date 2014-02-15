#include <gtest/gtest.h>
#include <memory>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

TEST(AeroFilterCase, ContentTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("s$");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 's');
    EXPECT_TRUE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^s");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 0);
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\d");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 63);
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\s");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 0);
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(s|t)");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 's' | 't');
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?:s|t)");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 's' | 't');
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?:s|\\d)");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 127);
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?=s)|d");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 0);
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?!s)|d");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(code->filter(), 0);
    EXPECT_FALSE(code->IsQuickCheckOneChar());
  }
}
