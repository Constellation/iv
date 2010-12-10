#include <gtest/gtest.h>
#include <string>
#include "cmdline.h"

TEST(CmdLineCase, CmdLineTest) {
  iv::cmdline::Parser cmd;
  cmd.Add("help", "help", 'h', "print this message");
  cmd.Add("version", "version", 'v', "print the version");
  cmd.Add("copyright", "copyright", '\0', "print the copyright");
  cmd.Add<std::string>("name", "name", 'n', "name", true, "");
  cmd.Add<int>("port", "port", 'p', "port number",
               false, 80, iv::cmdline::range(1, 65535));
  cmd.set_footer("[program_file] [arguments]");
  cmd.set_program_name("iv");
  bool result = cmd.Parse("command -n test -p 20 this is the test --help");

  const char* list[4] = {
    "this",
    "is",
    "the",
    "test"
  };
  EXPECT_TRUE(result);
  EXPECT_TRUE(cmd.Exist("help"));
  EXPECT_FALSE(cmd.Exist("version"));
  EXPECT_FALSE(cmd.Exist("copyright"));

  EXPECT_EQ(std::string("test"), cmd.get<std::string>("name"));
  EXPECT_EQ(20, cmd.get<int>("port"));
  EXPECT_TRUE(std::equal(list, list+4, cmd.rest().begin()));
}

TEST(CmdLineCase, StringTest) {
  iv::cmdline::Parser cmd;
  const char* list[4] = {
    "this",
    "is",
    "the",
    "test"
  };
  bool result = cmd.Parse("command this is the test");
  EXPECT_TRUE(result);
  EXPECT_TRUE(std::equal(list, list+4, cmd.rest().begin()));
}

TEST(CmdLineCase, NeedTest) {
  {
    iv::cmdline::Parser cmd;
    cmd.Add<std::string>("name", "name", 'n', "name", true, "");
    bool result = cmd.Parse("command this is the test");
    EXPECT_FALSE(result);
  }
  {
    iv::cmdline::Parser cmd;
    cmd.Add<std::string>("name", "name", 'n', "name", true, "");
    bool result = cmd.Parse("command -n this is the test");
    EXPECT_TRUE(result);
  }
  {
    iv::cmdline::Parser cmd;
    cmd.Add<std::string>("name", "", 'n', "name", true, "");
    bool result = cmd.Parse("command -n this is the test");
    EXPECT_TRUE(result);
  }
  {
    iv::cmdline::Parser cmd;
    cmd.Add<std::string>("name", "", 'n', "name", true, "");
    bool result = cmd.Parse("command --name this is the test");
    EXPECT_FALSE(result);
  }
}
