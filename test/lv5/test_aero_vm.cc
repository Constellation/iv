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
    ASSERT_TRUE(vm.Execute(str, &code, vec.data(), 0));
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
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    ASSERT_FALSE(vm.Execute(str1, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str2, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str3, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str4, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str5, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str6, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str7, &code, vec.data(), 0));
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
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    ASSERT_TRUE(vm.Execute(str1, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str2, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str3, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str4, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str5, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str6, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str7, &code, vec.data(), 0));
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
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    ASSERT_TRUE(vm.Execute(str1, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str2, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str3, &code, vec.data(), 0));
    ASSERT_TRUE(vm.Execute(str4, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str5, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str6, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str7, &code, vec.data(), 0));
  }
}

TEST(AeroVMCase, CaptureTest) {
  iv::core::Space space;
  iv::lv5::aero::VM vm;
  std::vector<int> vec(1000);
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
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    ASSERT_TRUE(vm.Execute(str1, &code, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(6, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(6, vec[3]);
    ASSERT_TRUE(vm.Execute(str2, &code, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(7, vec[1]);
    EXPECT_EQ(3, vec[2]);
    EXPECT_EQ(7, vec[3]);
    ASSERT_TRUE(vm.Execute(str3, &code, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(2, vec[2]);
    EXPECT_EQ(4, vec[3]);
    ASSERT_TRUE(vm.Execute(str4, &code, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(5, vec[1]);
    EXPECT_EQ(3, vec[2]);
    EXPECT_EQ(5, vec[3]);
    ASSERT_FALSE(vm.Execute(str5, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str6, &code, vec.data(), 0));
    ASSERT_FALSE(vm.Execute(str7, &code, vec.data(), 0));
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(?=(test))(test)");
    iv::core::UString str1 = iv::core::ToUString("test");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    ASSERT_TRUE(vm.Execute(str1, &code, vec.data(), 0));
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
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    ASSERT_TRUE(vm.Execute(str1, &code, vec.data(), 0));
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(0, vec[1]);
  }
  {
    space.Clear();
    iv::core::UString reg = iv::core::ToUString("(.*?)a(?!(a+)b\\2c)\\2(.*)");
    iv::core::UString str1 = iv::core::ToUString("baaabaac""");
    iv::lv5::aero::Parser parser(&space, reg, iv::lv5::aero::NONE);
    int error = 0;
    iv::lv5::aero::Disjunction* dis = parser.ParsePattern(&error);
    ASSERT_FALSE(error);
    iv::lv5::aero::Compiler compiler(iv::lv5::aero::NONE);
    iv::lv5::aero::Code code = compiler.Compile(dis);
    iv::lv5::aero::OutputDisAssembler disasm(stdout);
    disasm.DisAssemble(code.bytes());
    ASSERT_TRUE(vm.Execute(str1, &code, vec.data(), 0));
    EXPECT_EQ(0,  vec[0]);
    EXPECT_EQ(8,  vec[1]);
    EXPECT_EQ(0,  vec[2]);
    EXPECT_EQ(2,  vec[3]);
    EXPECT_EQ(-1, vec[4]);
    EXPECT_EQ(-1, vec[5]);
    EXPECT_EQ(3,  vec[6]);
    EXPECT_EQ(8,  vec[7]);
  }
}
