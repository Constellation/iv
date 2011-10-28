#include <gtest/gtest.h>
#include <vector>
#include "alloc.h"
#include "ustring.h"
#include "unicode.h"
#include "scoped_ptr.h"
#include "aero/aero.h"
#include "test_aero.h"

TEST(AeroVMCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a");
    iv::core::UString str = iv::core::ToUString("a");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("\\n");
    iv::core::UString str = iv::core::ToUString("\n");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[\\d]");
    iv::core::UString str = iv::core::ToUString("1");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[\\n]");
    iv::core::UString str = iv::core::ToUString("\n");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("s$");
    iv::core::UString str = iv::core::ToUString("s\n");
    iv::aero::Parser parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a*");
    iv::core::UString str = iv::core::ToUString("a");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^\\w$");
    iv::core::UString str = iv::core::ToUString("a");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^a{10,14}$");
    iv::core::UString str1 = iv::core::ToUString("aaaaaaaaa");
    iv::core::UString str2 = iv::core::ToUString("aaaaaaaaaa");
    iv::core::UString str3 = iv::core::ToUString("aaaaaaaaaaa");
    iv::core::UString str4 = iv::core::ToUString("aaaaaaaaaaaa");
    iv::core::UString str5 = iv::core::ToUString("aaaaaaaaaaaaa");
    iv::core::UString str6 = iv::core::ToUString("aaaaaaaaaaaaaa");
    iv::core::UString str7 = iv::core::ToUString("aaaaaaaaaaaaaaa");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str2, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str3, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str4, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str5, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str7, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^a{2,3}(?:test|ok)");
    iv::core::UString str1 = iv::core::ToUString("aatest");
    iv::core::UString str2 = iv::core::ToUString("aaatest");
    iv::core::UString str3 = iv::core::ToUString("aaok");
    iv::core::UString str4 = iv::core::ToUString("aaaok");
    iv::core::UString str5 = iv::core::ToUString("aaaaok");
    iv::core::UString str6 = iv::core::ToUString("aaaatest");
    iv::core::UString str7 = iv::core::ToUString("aaatess");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str2, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str3, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str4, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str5, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str7, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^a{2,3}(?:test|ok)");
    iv::core::UString str1 = iv::core::ToUString("aatest");
    iv::core::UString str2 = iv::core::ToUString("aaatest");
    iv::core::UString str3 = iv::core::ToUString("aaok");
    iv::core::UString str4 = iv::core::ToUString("aaaok");
    iv::core::UString str5 = iv::core::ToUString("aaaaok");
    iv::core::UString str6 = iv::core::ToUString("aaaatest");
    iv::core::UString str7 = iv::core::ToUString("aaatess");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str2, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str3, vec.data(), 0));
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str4, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str5, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str7, vec.data(), 0));
  }
}

TEST(AeroVMCase, CaptureTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^a{2,3}(test|ok)");
    iv::core::UString str1 = iv::core::ToUString("aatest");
    iv::core::UString str2 = iv::core::ToUString("aaatest");
    iv::core::UString str3 = iv::core::ToUString("aaok");
    iv::core::UString str4 = iv::core::ToUString("aaaok");
    iv::core::UString str5 = iv::core::ToUString("aaaaok");
    iv::core::UString str6 = iv::core::ToUString("aaaatest");
    iv::core::UString str7 = iv::core::ToUString("aaatess");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(6, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(6, vec[3]);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str2, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(7, vec[1]);
    EXPECT_EQ(3, vec[2]);
    EXPECT_EQ(7, vec[3]);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str3, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(4, vec[3]);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str4, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(5, vec[1]);
    EXPECT_EQ(3, vec[2]);
    EXPECT_EQ(5, vec[3]);
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str5, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.ExecuteOnce(code.get(), str7, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?=(test))(test)");
    iv::core::UString str1 = iv::core::ToUString("test");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(4, vec[3]);
    EXPECT_EQ(0, vec[4]);
    EXPECT_EQ(4, vec[5]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^$");
    iv::core::UString str1 = iv::core::ToUString("");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(0, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(.*?)a(?!(a+)b\\2c)\\2(.*)");
    iv::core::UString str1 = iv::core::ToUString("baaabaac");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0,  vec[0]);
    EXPECT_EQ(8,  vec[1]);
    EXPECT_EQ(0,  vec[2]);
    EXPECT_EQ(2,  vec[3]);
    EXPECT_EQ(-1, vec[4]);
    EXPECT_EQ(-1, vec[5]);
    EXPECT_EQ(3,  vec[6]);
    EXPECT_EQ(8,  vec[7]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(z)((a+)?(b+)?(c))*");
    iv::core::UString str1 = iv::core::ToUString("zaacbbbcac");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0,  vec[0]);
    EXPECT_EQ(10, vec[1]);
    EXPECT_EQ(0,  vec[2]);
    EXPECT_EQ(1,  vec[3]);
    EXPECT_EQ(8,  vec[4]);
    EXPECT_EQ(10, vec[5]);
    EXPECT_EQ(8,  vec[6]);
    EXPECT_EQ(9,  vec[7]);
    EXPECT_EQ(-1, vec[8]);
    EXPECT_EQ(-1, vec[9]);
    EXPECT_EQ(9,  vec[10]);
    EXPECT_EQ(10, vec[11]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(aa|aabaac|ba|b|c)*");
    iv::core::UString str1 = iv::core::ToUString("aabaac");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(4, vec[3]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a[a-z]{2,4}?");
    iv::core::UString str1 = iv::core::ToUString("abcdefghi");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a[a-z]{2,4}");
    iv::core::UString str1 = iv::core::ToUString("abcdefghi");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(5, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("a|ab");
    iv::core::UString str1 = iv::core::ToUString("abc");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("((a)|(ab))((c)|(bc))");
    iv::core::UString str1 = iv::core::ToUString("abc");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(1, vec[3]);
    EXPECT_EQ(0, vec[4]);
    EXPECT_EQ(1, vec[5]);
    EXPECT_EQ(-1, vec[6]);
    EXPECT_EQ(-1, vec[7]);
    EXPECT_EQ(1, vec[8]);
    EXPECT_EQ(3, vec[9]);
    EXPECT_EQ(-1, vec[10]);
    EXPECT_EQ(-1, vec[11]);
    EXPECT_EQ(1, vec[12]);
    EXPECT_EQ(3, vec[13]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("^(a+)\\1*,\\1+$");
    iv::core::UString str1 = iv::core::ToUString("aaaaaaaaaa,aaaaaaaaaaaaaaa");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(str1.size(), vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(5, vec[3]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(a*)*");
    iv::core::UString str1 = iv::core::ToUString("b");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(0, vec[1]);
    EXPECT_EQ(-1, vec[2]);
    EXPECT_EQ(-1, vec[3]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(a*)b\\1+");
    iv::core::UString str1 = iv::core::ToUString("baaaac");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(0, vec[3]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[-_.0-9A-Za-z]{1,64}@([-_0-9A-Za-z]){1,63}(.([-_.0-9A-Za-z]{1,63}))");
    iv::core::UString str1 = iv::core::ToUString("utatane.tea@gmail.com");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString(kURLRegExp);
    iv::core::UString str1 = iv::core::ToUString("http://github.com/Constellation/");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(32, vec[1]);
    EXPECT_EQ(7, vec[2]);
    EXPECT_EQ(17, vec[3]);
    EXPECT_EQ(7, vec[4]);
    EXPECT_EQ(14, vec[5]);
    EXPECT_EQ(7, vec[6]);
    EXPECT_EQ(13, vec[7]);
    EXPECT_EQ(14, vec[8]);
    EXPECT_EQ(17, vec[9]);
    EXPECT_EQ(-1, vec[10]);
    EXPECT_EQ(-1, vec[11]);
    EXPECT_EQ(17, vec[12]);
    EXPECT_EQ(32, vec[13]);
    EXPECT_EQ(18, vec[14]);
    EXPECT_EQ(31, vec[15]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[a-z][^1-9][a-z]");
    iv::core::UString str1 = iv::core::ToUString("def");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[^1-9]");
    iv::core::UString str1 = iv::core::ToUString("d");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("[\\d][\\n][^\\d]");
    iv::core::UString str1 = iv::core::ToUString("1\nline");
    iv::aero::Parser parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    iv::core::ScopedPtr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.ExecuteOnce(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
  }
}
