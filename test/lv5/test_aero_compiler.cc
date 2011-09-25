#include <gtest/gtest.h>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "lv5/aero/parser.h"
#include "lv5/aero/compiler.h"
#include "lv5/aero/disassembler.h"

TEST(AeroCompilerCase, MainTest) {
  iv::core::Space space;
  {
    space.Clear();
    iv::lv5::aero::OutputDisAssembler disasm(stdout);
    iv::core::UString str = iv::core::ToUString("^ma[^a-d]$");
    iv::lv5::aero::Parser parser(&space, str, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    disasm.DisAssemble(compiler.Compile(dis));
  }
}
