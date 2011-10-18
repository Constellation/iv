#include <gtest/gtest.h>
#include <vector>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "lv5/aero/aero.h"
#include "test_aero.h"

TEST(AeroExecCase, MainTest) {
  iv::core::Space space;
  iv::lv5::aero::VM vm;
  std::vector<int> vec(1000);
  iv::lv5::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("s$");
    iv::core::UString str1 = iv::core::ToUString("s\n");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::MULTILINE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::MULTILINE);
    iv::lv5::aero::Code code = compiler.Compile(data);
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(&code, str1, 0, vec.data()));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^\\d+");
    iv::core::UString str1 = iv::core::ToUString("abc\n123xyz");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::MULTILINE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::MULTILINE);
    iv::lv5::aero::Code code = compiler.Compile(data);
    ASSERT_TRUE(vm.ExecuteOnce(&code, str1, 0, vec.data()));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("b{0,93}c");
    iv::core::UString str1 = iv::core::ToUString("aaabbbbcccddeeeefffff");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(data);
    ASSERT_TRUE(vm.ExecuteOnce(&code, str1, 0, vec.data()));
    EXPECT_EQ(3, vec[0]);
    EXPECT_EQ(8, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\b(\\w+) \\2\\b");
    iv::core::UString str1 = iv::core::ToUString("do you listen the the band");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(data);
    // disasm.DisAssemble(code);
    // ASSERT_FALSE(vm.ExecuteOnce(&code, str1, 0, vec.data()));
  }
}
