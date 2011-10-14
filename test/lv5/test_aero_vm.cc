#include <gtest/gtest.h>
#include <vector>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "lv5/aero/parser.h"
#include "lv5/aero/compiler.h"
#include "lv5/aero/disassembler.h"
#include "lv5/aero/vm.h"

TEST(AeroVMCase, MainTest) {
  iv::core::Space space;
  iv::lv5::aero::VM vm;
  std::vector<int> vec(1000);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a");
    iv::core::UString str = iv::core::ToUString("a");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    iv::lv5::aero::OutputDisAssembler disasm(stdout);
    disasm.DisAssemble(code.bytes());
    ASSERT_TRUE(vm.Execute(str, &code, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a*");
    iv::core::UString str = iv::core::ToUString("a");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    iv::lv5::aero::OutputDisAssembler disasm(stdout);
    disasm.DisAssemble(code.bytes());
    ASSERT_TRUE(vm.Execute(str, &code, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^\\w$");
    iv::core::UString str = iv::core::ToUString("a");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    iv::lv5::aero::OutputDisAssembler disasm(stdout);
    disasm.DisAssemble(code.bytes());
    ASSERT_TRUE(vm.Execute(str, &code, vec.data(), 0));
  }
}
