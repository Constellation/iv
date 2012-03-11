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
    iv::aero::JIT<uint16_t> jit(*code.get());
    jit.Compile();

    for (std::size_t i = 0; i < 100000; ++i)
    jit.Get()(&vm, str1.data(), str1.size(), vec.data(), 0);
    iv::aero::JIT<uint16_t>::Executable exe = jit.Get();
    std::cout << exe(&vm, str1.data(), str1.size(), vec.data(), 0) << std::endl;
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
    iv::aero::JIT<char> jit(*code.get());
    jit.Compile();

    for (std::size_t i = 0; i < 100000; ++i)
    jit.Get()(&vm, str1.data(), str1.size(), vec.data(), 0);
    iv::aero::JIT<char>::Executable exe = jit.Get();
    std::cout << exe(&vm, str1.data(), str1.size(), vec.data(), 0) << std::endl;
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
    iv::aero::JIT<uint16_t> jit(*code.get());
    jit.Compile();
    iv::aero::JIT<uint16_t>::Executable exe = jit.Get();
    std::cout << exe(&vm, str1.data(), str1.size(), vec.data(), 0) << std::endl;
  }
}
#endif
