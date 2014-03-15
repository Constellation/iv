#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <string>
#include <iv/alloc.h>
#include <iv/string.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

TEST(AeroExecCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("s$");
    std::u16string str1 = iv::core::ToU16String("s\n");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^\\d+");
    std::u16string str1 = iv::core::ToU16String("abc\n123xyz");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("b{0,93}c");
    std::u16string str1 = iv::core::ToU16String("aaabbbbcccddeeeefffff");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(3, vec[0]);
    EXPECT_EQ(8, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\b(\\w+) \\2\\b");
    std::u16string str1 = iv::core::ToU16String("do you listen the the band");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_FALSE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\1(A)");
    std::u16string str1 = iv::core::ToU16String("AA");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\s");
    std::u16string str = iv::core::ToU16String(0xFEFF);
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
}

TEST(AeroExecCase, CounterTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("a{2,3}");
    std::u16string str1 = iv::core::ToU16String("aa");
    std::u16string str2 = iv::core::ToU16String("aaa");
    std::u16string str3 = iv::core::ToU16String("a");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str3, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^a+$");
    std::u16string str1 = iv::core::ToU16String("aa");
    std::u16string str2 = iv::core::ToU16String("aaa");
    std::u16string str3 = iv::core::ToU16String("");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str3, vec.data(), 0));
  }
  // Extracted from SunSpider
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[\\0\\xa0]");
    std::u16string str1 = iv::core::ToU16String('\0');
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
}
