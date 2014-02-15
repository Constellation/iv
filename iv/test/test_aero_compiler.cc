#include <gtest/gtest.h>
#include <memory>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>

TEST(AeroCompilerCase, MainTest) {
  iv::core::Space space;
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("a*?");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("t+");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::IGNORE_CASE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("[\\u0000-\\uFFFF]");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::IGNORE_CASE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[^]]*]([^]]+])*]+");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\u10");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\u");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
  }
  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("(?=\\d)(\\d{3})(\\d{3})+$");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    ASSERT_TRUE(data.pattern());
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    EXPECT_EQ(3, code->captures());
  }
}
