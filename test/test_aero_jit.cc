#include <gtest/gtest.h>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/scoped_ptr.h>
#include <iv/aero/aero.h>
#include <iv/aero/jit.h>

#if defined(IV_ENABLE_JIT)
TEST(AeroJITCase, MainTest) {
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
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    iv::aero::JIT jit;
    jit.Compile(*code.get());
  }
}
#endif
