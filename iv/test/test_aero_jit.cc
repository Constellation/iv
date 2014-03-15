#include <gtest/gtest.h>
#include <memory>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include <iv/aero/jit.h>
#include "test_aero.h"

#if defined(IV_ENABLE_JIT)
TEST(AeroJITCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("a*?");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    iv::aero::JIT<char16_t> jit(*code.get());
    jit.Compile();
  }

  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("(a){2}");
    std::u16string str1 = iv::core::ToU16String("aa");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));

    for (std::size_t i = 0; i < 100000; ++i)
      vm.Execute(code.get(), str1, vec.data(), 0);
  }

  {
    space.Clear();
    std::u16string str = iv::core::ToU16String("(a){2}");
    std::string str1("aa");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    for (std::size_t i = 0; i < 100000; ++i) {
      vm.Execute(code.get(), str1, vec.data(), 0);
    }
  }

  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String(kURLRegExp);
    std::u16string str1 = iv::core::ToU16String("http://github.com/Constellation/");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    vm.Execute(code.get(), str1, vec.data(), 0);
  }

  // Found by SunSpider
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[\\0\\xa0]");
    std::u16string str1 = iv::core::ToU16String('\0');
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    vm.Execute(code.get(), str1, vec.data(), 0);
  }
}


TEST(AeroJITCase, SSE42) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  space.Clear();

  // SSE4.2 range short
  {
    std::u16string reg = iv::core::ToU16String("[abcd]");
    std::u16string str1 = iv::core::ToU16String('a');
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    vm.Execute(code.get(), str1, vec.data(), 0);
  }


  // SSE4.2 range long
  {
    std::u16string reg = iv::core::ToU16String("[a-cf-zA-FI-PT-Z]");
    std::u16string str1 = iv::core::ToU16String('Z');
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    vm.Execute(code.get(), str1, vec.data(), 0);
  }
}


TEST(AeroJITCase, FailedAtTest262Test) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\u0FFF");
    std::u16string str1 = iv::core::ToU16String(0x0FFF);
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    // disasm.DisAssemble(*code.get());
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\u7FFF");
    std::u16string str1 = iv::core::ToU16String(0x7FFF);
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    // disasm.DisAssemble(*code.get());
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\BE");
    std::u16string str1 = iv::core::ToU16String("TEST");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    // disasm.DisAssemble(*code.get());
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(2, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("Java(Script)?(?=\\:)");
    std::u16string str1 = iv::core::ToU16String("just JavaScript:");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    // disasm.DisAssemble(*code.get());
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(5, vec[0]);
    EXPECT_EQ(15, vec[1]);
    EXPECT_EQ(9, vec[2]);
    EXPECT_EQ(15, vec[3]);
  }
}
#endif
