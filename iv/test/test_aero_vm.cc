#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <string>
#include <iv/alloc.h>
#include <iv/string.h>
#include <iv/unicode.h>
#include <iv/aero/aero.h>
#include "test_aero.h"

TEST(AeroVMCase, MainTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("a");
    std::u16string str = iv::core::ToU16String("a");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("\\n");
    std::u16string str = iv::core::ToU16String("\n");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[\\d]");
    std::u16string str = iv::core::ToU16String("1");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[\\n]");
    std::u16string str = iv::core::ToU16String("\n");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("s$");
    std::u16string str = iv::core::ToU16String("s\n");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::MULTILINE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::MULTILINE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("a*");
    std::u16string str = iv::core::ToU16String("a");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^\\w$");
    std::u16string str = iv::core::ToU16String("a");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.Execute(code.get(), str, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^a{10,14}$");
    std::u16string str1 = iv::core::ToU16String("aaaaaaaaa");
    std::u16string str2 = iv::core::ToU16String("aaaaaaaaaa");
    std::u16string str3 = iv::core::ToU16String("aaaaaaaaaaa");
    std::u16string str4 = iv::core::ToU16String("aaaaaaaaaaaa");
    std::u16string str5 = iv::core::ToU16String("aaaaaaaaaaaaa");
    std::u16string str6 = iv::core::ToU16String("aaaaaaaaaaaaaa");
    std::u16string str7 = iv::core::ToU16String("aaaaaaaaaaaaaaa");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_FALSE(vm.Execute(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str3, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str4, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str5, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str7, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^a{2,3}(?:test|ok)");
    std::u16string str1 = iv::core::ToU16String("aatest");
    std::u16string str2 = iv::core::ToU16String("aaatest");
    std::u16string str3 = iv::core::ToU16String("aaok");
    std::u16string str4 = iv::core::ToU16String("aaaok");
    std::u16string str5 = iv::core::ToU16String("aaaaok");
    std::u16string str6 = iv::core::ToU16String("aaaatest");
    std::u16string str7 = iv::core::ToU16String("aaatess");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str3, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str4, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str5, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str7, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^a{2,3}(?:test|ok)");
    std::u16string str1 = iv::core::ToU16String("aatest");
    std::u16string str2 = iv::core::ToU16String("aaatest");
    std::u16string str3 = iv::core::ToU16String("aaok");
    std::u16string str4 = iv::core::ToU16String("aaaok");
    std::u16string str5 = iv::core::ToU16String("aaaaok");
    std::u16string str6 = iv::core::ToU16String("aaaatest");
    std::u16string str7 = iv::core::ToU16String("aaatess");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str3, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(code.get(), str4, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str5, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str7, vec.data(), 0));
  }
}

TEST(AeroVMCase, CaptureTest) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^a{2,3}(test|ok)");
    std::u16string str1 = iv::core::ToU16String("aatest");
    std::u16string str2 = iv::core::ToU16String("aaatest");
    std::u16string str3 = iv::core::ToU16String("aaok");
    std::u16string str4 = iv::core::ToU16String("aaaok");
    std::u16string str5 = iv::core::ToU16String("aaaaok");
    std::u16string str6 = iv::core::ToU16String("aaaatest");
    std::u16string str7 = iv::core::ToU16String("aaatess");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(*code.get());
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(6, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(6, vec[3]);
    ASSERT_TRUE(vm.Execute(code.get(), str2, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(7, vec[1]);
    EXPECT_EQ(3, vec[2]);
    EXPECT_EQ(7, vec[3]);
    ASSERT_TRUE(vm.Execute(code.get(), str3, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(4, vec[3]);
    ASSERT_TRUE(vm.Execute(code.get(), str4, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(5, vec[1]);
    EXPECT_EQ(3, vec[2]);
    EXPECT_EQ(5, vec[3]);
    ASSERT_FALSE(vm.Execute(code.get(), str5, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str6, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(code.get(), str7, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("(?=(test))(test)");
    std::u16string str1 = iv::core::ToU16String("test");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(4, vec[3]);
    EXPECT_EQ(0, vec[4]);
    EXPECT_EQ(4, vec[5]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("^$");
    std::u16string str1 = iv::core::ToU16String("");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(0, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("(.*?)a(?!(a+)b\\2c)\\2(.*)");
    std::u16string str1 = iv::core::ToU16String("baaabaac");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
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
    std::u16string reg = iv::core::ToU16String("a[a-z]{2,4}?");
    std::u16string str1 = iv::core::ToU16String("abcdefghi");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("a[a-z]{2,4}");
    std::u16string str1 = iv::core::ToU16String("abcdefghi");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(5, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("a|ab");
    std::u16string str1 = iv::core::ToU16String("abc");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("((a)|(ab))((c)|(bc))");
    std::u16string str1 = iv::core::ToU16String("abc");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
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
    std::u16string reg = iv::core::ToU16String("^(a+)\\1*,\\1+$");
    std::u16string str1 = iv::core::ToU16String("aaaaaaaaaa,aaaaaaaaaaaaaaa");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(str1.size(), static_cast<std::size_t>(vec[1]));
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(5, vec[3]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("(a*)*");
    std::u16string str1 = iv::core::ToU16String("b");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(0, vec[1]);
    EXPECT_EQ(-1, vec[2]);
    EXPECT_EQ(-1, vec[3]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("(a*)b\\1+");
    std::u16string str1 = iv::core::ToU16String("baaaac");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(0, vec[3]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[-_.0-9A-Za-z]{1,64}@([-_0-9A-Za-z]){1,63}(.([-_.0-9A-Za-z]{1,63}))");
    std::u16string str1 = iv::core::ToU16String("utatane.tea@gmail.com");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[a-z][^1-9][a-z]");
    std::u16string str1 = iv::core::ToU16String("def");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[^1-9]");
    std::u16string str1 = iv::core::ToU16String("d");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1, vec[1]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("[\\d][\\n][^\\d]");
    std::u16string str1 = iv::core::ToU16String("1\nline");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(3, vec[1]);
  }
}

TEST(AeroVMCase, CaptureTest2) {
  iv::core::Space space;
  iv::aero::VM vm;
  std::vector<int> vec(1000);
  iv::aero::OutputDisAssembler disasm(stdout);
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String("(z)((a+)?(b+)?(c))*");
    std::u16string str1 = iv::core::ToU16String("zaacbbbcac");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
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
    std::u16string reg = iv::core::ToU16String("(aa|aabaac|ba|b|c)*");
    std::u16string str1 = iv::core::ToU16String("aabaac");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(4, vec[3]);
  }
  {
    space.Clear();
    std::u16string reg = iv::core::ToU16String(kURLRegExp);
    std::u16string str1 = iv::core::ToU16String("http://github.com/Constellation/");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(code);
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
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
    std::u16string reg = iv::core::ToU16String("\\B(?=(?:\\d{3})+$)");
    std::u16string str1 = iv::core::ToU16String("10000");
    iv::aero::Parser<iv::core::u16string_view> parser(&space, reg, iv::aero::NONE);
    int error = 0;
    iv::aero::ParsedData data = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::aero::Compiler compiler(iv::aero::NONE);
    std::unique_ptr<iv::aero::Code> code(compiler.Compile(data));;
    // disasm.DisAssemble(*code);
    EXPECT_EQ(1, code->captures());
    ASSERT_TRUE(vm.Execute(code.get(), str1, vec.data(), 0));
  }
}
