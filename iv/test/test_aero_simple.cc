#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <string>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

TEST(AeroSimpleCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("s");
    std::u16string str1 = iv::core::ToU16String("s\n");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
}

TEST(AeroSimpleCase, CounterTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("aa");
    std::u16string str1 = iv::core::ToU16String("aa");
    std::u16string str2 = iv::core::ToU16String("aaa");
    std::u16string str3 = iv::core::ToU16String("a");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str3, vec.data(), 0));
  }
}
