#include <gtest/gtest.h>
#include <iv/alloc.h>
#include <iv/ustring.h>
#include <iv/unicode.h>
#include <iv/scoped_ptr.h>
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
    iv::core::UString str = iv::core::ToUString("a*?");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    iv::aero::JIT<uint16_t> jit(*code.get());
    jit.Compile();
  }

  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("(a){2}");
    iv::core::UString str1 = iv::core::ToUString("aa");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));

    for (std::size_t i = 0; i < 100000; ++i)
      vm.Execute(code.get(), str1, vec.data(), 0);
  }

  {
    space.Clear();
    iv::core::UString str = iv::core::ToUString("(a){2}");
    std::string str1("aa");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, str, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    for (std::size_t i = 0; i < 100000; ++i) {
      vm.Execute(code.get(), str1, vec.data(), 0);
    }
  }

  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString(kURLRegExp);
    iv::core::UString str1 = iv::core::ToUString("http://github.com/Constellation/");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    vm.Execute(code.get(), str1, vec.data(), 0);
  }
}
#endif
