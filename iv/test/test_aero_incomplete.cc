#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <string>
#include <iv/alloc.h>
#include <iv/string.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

// see
// https://mail.mozilla.org/pipermail/es-discuss/2011-October/017595.html
// http://wiki.ecmascript.org/doku.php?id=harmony:regexp_match_web_reality
// http://wiki.ecmascript.org/doku.php?id=strawman:match_web_reality_spec

TEST(AeroIncompleteCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^\\c2$");
    std::u16string str1 = iv::core::ToU16String("c2");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^\\x2$");
    std::u16string str1 = iv::core::ToU16String("x2");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
}

TEST(AeroIncompleteCase, UnicodeEscapeSequenceTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);

  typedef std::array<std::pair<std::u16string, std::u16string>, 7> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToU16String("^\\uz$"),    iv::core::ToU16String("uz")),
    std::make_pair(iv::core::ToU16String("^\\u1$"),    iv::core::ToU16String("u1")),
    std::make_pair(iv::core::ToU16String("^\\u1z$"),   iv::core::ToU16String("u1z")),
    std::make_pair(iv::core::ToU16String("^\\u12$"),   iv::core::ToU16String("u12")),
    std::make_pair(iv::core::ToU16String("^\\u12z$"),  iv::core::ToU16String("u12z")),
    std::make_pair(iv::core::ToU16String("^\\u123$"),  iv::core::ToU16String("u123")),
    std::make_pair(iv::core::ToU16String("^\\u123z$"), iv::core::ToU16String("u123z"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, it->first, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), it->second, vec.data(), 0));
  }
}

TEST(AeroIncompleteCase, BadQuantifierRangeTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);

  typedef std::array<std::pair<std::u16string, std::u16string>, 4> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToU16String("^x{1z$"),    iv::core::ToU16String("x{1z")),
    std::make_pair(iv::core::ToU16String("^x{1,z$"),    iv::core::ToU16String("x{1,z")),
    std::make_pair(iv::core::ToU16String("^x{1,2z$"),   iv::core::ToU16String("x{1,2z")),
    std::make_pair(iv::core::ToU16String("^x{10000,20000z$"),   iv::core::ToU16String("x{10000,20000z"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, it->first, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), it->second, vec.data(), 0));
  }
}

TEST(AeroIncompleteCase, ClassEscapeTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);

  typedef std::array<std::pair<std::u16string, std::u16string>, 16> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToU16String("^[\\z]$"),    iv::core::ToU16String("z")),
    std::make_pair(iv::core::ToU16String("^[\\c]$"),    iv::core::ToU16String("c")),
    std::make_pair(iv::core::ToU16String("^[\\c2]+$"),   iv::core::ToU16String("c2")),
    std::make_pair(iv::core::ToU16String("^[\\x]$"),   iv::core::ToU16String("x")),
    std::make_pair(iv::core::ToU16String("^[\\x1z]+$"),  iv::core::ToU16String("x1z")),
    std::make_pair(iv::core::ToU16String("^[\\u]$"),  iv::core::ToU16String("u")),
    std::make_pair(iv::core::ToU16String("^[\\u]$"),  iv::core::ToU16String("u")),
    std::make_pair(iv::core::ToU16String("^[\\uz]+$"),  iv::core::ToU16String("uz")),
    std::make_pair(iv::core::ToU16String("^[\\u1]+$"),  iv::core::ToU16String("u1")),
    std::make_pair(iv::core::ToU16String("^[\\u1z]+$"),  iv::core::ToU16String("u1z")),
    std::make_pair(iv::core::ToU16String("^[\\u12]+$"),  iv::core::ToU16String("u12")),
    std::make_pair(iv::core::ToU16String("^[\\u12z]+$"),  iv::core::ToU16String("u12z")),
    std::make_pair(iv::core::ToU16String("^[\\u123]+$"),  iv::core::ToU16String("u123")),
    std::make_pair(iv::core::ToU16String("^[\\u123z]+$"),  iv::core::ToU16String("u123z")),
    std::make_pair(iv::core::ToU16String("^[\\012]$"),  iv::core::ToU16String("\n")),
    std::make_pair(iv::core::ToU16String("^[\\5]$"),  iv::core::ToU16String(5))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, it->first, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error) << std::distance(kMatchers.begin(), it) << "TEST";
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), it->second, vec.data(), 0)) << std::distance(kMatchers.begin(), it) << "TEST";
  }
}

TEST(AeroIncompleteCase, CounterTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^[\\c2]+$");
    std::u16string str1 = iv::core::ToU16String("c2");
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
}

// see http://wiki.ecmascript.org/doku.php?id=strawman:match_web_reality_spec
TEST(AeroIncompleteCase, EscapeMissTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);

  typedef std::array<std::pair<std::u16string, std::u16string>, 17> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToU16String("^\\02$"),    iv::core::ToU16String(2)),
    std::make_pair(iv::core::ToU16String("^[\\02]$"),    iv::core::ToU16String(2)),
    std::make_pair(iv::core::ToU16String("^\\12$"),    iv::core::ToU16String("\n")),
    std::make_pair(iv::core::ToU16String("^[\\12]$"),    iv::core::ToU16String("\n")),
    std::make_pair(iv::core::ToU16String("^\\xg$"),   iv::core::ToU16String("xg")),
    std::make_pair(iv::core::ToU16String("^[\\xg]+$"),   iv::core::ToU16String("xg")),
    std::make_pair(iv::core::ToU16String("^\\ug$"),   iv::core::ToU16String("ug")),
    std::make_pair(iv::core::ToU16String("^[\\ug]+$"),   iv::core::ToU16String("ug")),
    std::make_pair(iv::core::ToU16String("^\\a$"),   iv::core::ToU16String("a")),
    std::make_pair(iv::core::ToU16String("^[\\a]$"),   iv::core::ToU16String("a")),
    std::make_pair(iv::core::ToU16String("^\\8$"),   iv::core::ToU16String(8)),
    std::make_pair(iv::core::ToU16String("^[\\8]$"),   iv::core::ToU16String("8")),
    std::make_pair(iv::core::ToU16String("(?!a)+"),   iv::core::ToU16String("")),
    std::make_pair(iv::core::ToU16String("{"),   iv::core::ToU16String("{")),
    std::make_pair(iv::core::ToU16String("}"),   iv::core::ToU16String("}")),
    std::make_pair(iv::core::ToU16String("\\c1"),   iv::core::ToU16String("c1")),
    std::make_pair(iv::core::ToU16String("^[\\c1]+$"),   iv::core::ToU16String("c1"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, it->first, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error) << std::distance(kMatchers.begin(), it) << "TEST";
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), it->second, vec.data(), 0)) << std::distance(kMatchers.begin(), it) << "TEST";
  }
}

TEST(AeroIncompleteCase, EscapeMissFailTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);

  typedef std::array<std::pair<std::u16string, std::u16string>, 7> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToU16String("^[\\02]$"),    iv::core::ToU16String('\\')),
    std::make_pair(iv::core::ToU16String("^[\\12]$"),    iv::core::ToU16String('\\')),
    std::make_pair(iv::core::ToU16String("^[\\xg]$"),   iv::core::ToU16String("\\")),
    std::make_pair(iv::core::ToU16String("^[\\ug]$"),   iv::core::ToU16String("\\")),
    std::make_pair(iv::core::ToU16String("^[\\a]$"),   iv::core::ToU16String("\\")),
    std::make_pair(iv::core::ToU16String("^[\\8]$"),   iv::core::ToU16String("\\")),
    std::make_pair(iv::core::ToU16String("^[\\c1]$"),   iv::core::ToU16String("\\"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::U16StringPiece> parser(&space, it->first, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_FALSE(vm.Execute(code.get(), it->second, vec.data(), 0));
  }
}
