#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <iv/alloc.h>
#include <iv/ustring.h>
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
    iv::core::UString reg = iv::core::ToUString("^\\c2$");
    iv::core::UString str1 = iv::core::ToUString("c2");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^\\x2$");
    iv::core::UString str1 = iv::core::ToUString("x2");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::MULTILINE);
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

  typedef std::array<std::pair<iv::core::UString, iv::core::UString>, 7> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToUString("^\\uz$"),    iv::core::ToUString("uz")),
    std::make_pair(iv::core::ToUString("^\\u1$"),    iv::core::ToUString("u1")),
    std::make_pair(iv::core::ToUString("^\\u1z$"),   iv::core::ToUString("u1z")),
    std::make_pair(iv::core::ToUString("^\\u12$"),   iv::core::ToUString("u12")),
    std::make_pair(iv::core::ToUString("^\\u12z$"),  iv::core::ToUString("u12z")),
    std::make_pair(iv::core::ToUString("^\\u123$"),  iv::core::ToUString("u123")),
    std::make_pair(iv::core::ToUString("^\\u123z$"), iv::core::ToUString("u123z"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, it->first, iv::aero::NONE);
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

  typedef std::array<std::pair<iv::core::UString, iv::core::UString>, 4> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToUString("^x{1z$"),    iv::core::ToUString("x{1z")),
    std::make_pair(iv::core::ToUString("^x{1,z$"),    iv::core::ToUString("x{1,z")),
    std::make_pair(iv::core::ToUString("^x{1,2z$"),   iv::core::ToUString("x{1,2z")),
    std::make_pair(iv::core::ToUString("^x{10000,20000z$"),   iv::core::ToUString("x{10000,20000z"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, it->first, iv::aero::NONE);
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

  typedef std::array<std::pair<iv::core::UString, iv::core::UString>, 16> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToUString("^[\\z]$"),    iv::core::ToUString("z")),
    std::make_pair(iv::core::ToUString("^[\\c]$"),    iv::core::ToUString("c")),
    std::make_pair(iv::core::ToUString("^[\\c2]+$"),   iv::core::ToUString("c2")),
    std::make_pair(iv::core::ToUString("^[\\x]$"),   iv::core::ToUString("x")),
    std::make_pair(iv::core::ToUString("^[\\x1z]+$"),  iv::core::ToUString("x1z")),
    std::make_pair(iv::core::ToUString("^[\\u]$"),  iv::core::ToUString("u")),
    std::make_pair(iv::core::ToUString("^[\\u]$"),  iv::core::ToUString("u")),
    std::make_pair(iv::core::ToUString("^[\\uz]+$"),  iv::core::ToUString("uz")),
    std::make_pair(iv::core::ToUString("^[\\u1]+$"),  iv::core::ToUString("u1")),
    std::make_pair(iv::core::ToUString("^[\\u1z]+$"),  iv::core::ToUString("u1z")),
    std::make_pair(iv::core::ToUString("^[\\u12]+$"),  iv::core::ToUString("u12")),
    std::make_pair(iv::core::ToUString("^[\\u12z]+$"),  iv::core::ToUString("u12z")),
    std::make_pair(iv::core::ToUString("^[\\u123]+$"),  iv::core::ToUString("u123")),
    std::make_pair(iv::core::ToUString("^[\\u123z]+$"),  iv::core::ToUString("u123z")),
    std::make_pair(iv::core::ToUString("^[\\012]$"),  iv::core::ToUString("\n")),
    std::make_pair(iv::core::ToUString("^[\\5]$"),  iv::core::ToUString(5))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, it->first, iv::aero::NONE);
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
    iv::core::UString reg = iv::core::ToUString("^[\\c2]+$");
    iv::core::UString str1 = iv::core::ToUString("c2");
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, reg, iv::aero::NONE);
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

  typedef std::array<std::pair<iv::core::UString, iv::core::UString>, 17> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToUString("^\\02$"),    iv::core::ToUString(2)),
    std::make_pair(iv::core::ToUString("^[\\02]$"),    iv::core::ToUString(2)),
    std::make_pair(iv::core::ToUString("^\\12$"),    iv::core::ToUString("\n")),
    std::make_pair(iv::core::ToUString("^[\\12]$"),    iv::core::ToUString("\n")),
    std::make_pair(iv::core::ToUString("^\\xg$"),   iv::core::ToUString("xg")),
    std::make_pair(iv::core::ToUString("^[\\xg]+$"),   iv::core::ToUString("xg")),
    std::make_pair(iv::core::ToUString("^\\ug$"),   iv::core::ToUString("ug")),
    std::make_pair(iv::core::ToUString("^[\\ug]+$"),   iv::core::ToUString("ug")),
    std::make_pair(iv::core::ToUString("^\\a$"),   iv::core::ToUString("a")),
    std::make_pair(iv::core::ToUString("^[\\a]$"),   iv::core::ToUString("a")),
    std::make_pair(iv::core::ToUString("^\\8$"),   iv::core::ToUString(8)),
    std::make_pair(iv::core::ToUString("^[\\8]$"),   iv::core::ToUString("8")),
    std::make_pair(iv::core::ToUString("(?!a)+"),   iv::core::ToUString("")),
    std::make_pair(iv::core::ToUString("{"),   iv::core::ToUString("{")),
    std::make_pair(iv::core::ToUString("}"),   iv::core::ToUString("}")),
    std::make_pair(iv::core::ToUString("\\c1"),   iv::core::ToUString("c1")),
    std::make_pair(iv::core::ToUString("^[\\c1]+$"),   iv::core::ToUString("c1"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, it->first, iv::aero::NONE);
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

  typedef std::array<std::pair<iv::core::UString, iv::core::UString>, 7> Matchers;
  const Matchers kMatchers = { {
    std::make_pair(iv::core::ToUString("^[\\02]$"),    iv::core::ToUString('\\')),
    std::make_pair(iv::core::ToUString("^[\\12]$"),    iv::core::ToUString('\\')),
    std::make_pair(iv::core::ToUString("^[\\xg]$"),   iv::core::ToUString("\\")),
    std::make_pair(iv::core::ToUString("^[\\ug]$"),   iv::core::ToUString("\\")),
    std::make_pair(iv::core::ToUString("^[\\a]$"),   iv::core::ToUString("\\")),
    std::make_pair(iv::core::ToUString("^[\\8]$"),   iv::core::ToUString("\\")),
    std::make_pair(iv::core::ToUString("^[\\c1]$"),   iv::core::ToUString("\\"))
  } };

  for (Matchers::const_iterator it = kMatchers.begin(),
       last = kMatchers.end(); it != last; ++it) {
    space.Clear();
    iv::aero::Parser<iv::core::UStringPiece> parser(&space, it->first, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_FALSE(vm.Execute(code.get(), it->second, vec.data(), 0));
  }
}
