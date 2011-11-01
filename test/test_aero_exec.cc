#include <gtest/gtest.h>
#include <vector>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/scoped_ptr.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

TEST(AeroExecCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("s$");
    iv::core::UString str1 = iv::core::ToUString("s\n");
    iv::aero::Parser parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^\\d+");
    iv::core::UString str1 = iv::core::ToUString("abc\n123xyz");
    iv::aero::Parser parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("b{0,93}c");
    iv::core::UString str1 = iv::core::ToUString("aaabbbbcccddeeeefffff");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(3, vec[0]);
    EXPECT_EQ(8, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\b(\\w+) \\2\\b");
    iv::core::UString str1 = iv::core::ToUString("do you listen the the band");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\1(A)");
    iv::core::UString str1 = iv::core::ToUString("AA");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
  }
}
