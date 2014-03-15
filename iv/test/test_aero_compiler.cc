#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <iv/alloc.h>
#include <iv/string.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>

TEST(AeroCompilerCase, MainTest) {
  iv::core::Space space;
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a*?");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("t+");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, str, iv::aero::IGNORE_CASE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("[\\u0000-\\uFFFF]");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, str, iv::aero::IGNORE_CASE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[^]]*]([^]]+])*]+");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\u10");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\u");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("(?=\\d)(\\d{3})(\\d{3})+$");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(3, code->captures());
  }
}
