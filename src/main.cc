#include <cstdio>
#include <cstdlib>
#include <locale>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>  // NOLINT
#include "config.h"
#include "cmdline.h"
#include "ast.h"
#include "ast-serializer.h"
#include "codegen.h"
#include "parser.h"

int main(int argc, char **argv) {
  std::locale::global(std::locale(""));

  iv::cmdline::Parser cmd("iv");

  cmd.Add("help",
          "help",
          'h', "print this message");
  cmd.Add("version",
          "version",
          'v', "print the version");
  cmd.Add("warning",
          "",
          'W', "set warning level 0 - 2", false, 0, iv::cmdline::range(0, 2));
  cmd.Add("ast",
          "ast",
          0, "print ast");
  cmd.Add("copyright",
          "copyright",
          0,   "print the copyright");
  cmd.set_footer("[program_file] [arguments]");

  bool cmd_parse_success = cmd.Parse(argc, argv);
  if (!cmd_parse_success) {
    std::cerr << cmd.error() << std::endl << cmd.usage();
    return EXIT_FAILURE;
  }

  if (argc == 1 || cmd.Exist("help")) {
    std::cout << cmd.usage();
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("version")) {
    printf("iv %s (compiled %s %s)\n", IV_VERSION, __DATE__, __TIME__);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("copyright")) {
    printf("iv - Copyright (C) 2010 %s\n", IV_DEVELOPER);
    return EXIT_SUCCESS;
  }

  const std::vector<std::string>& rest = cmd.rest();
  if (!rest.empty()) {
    std::string str;
    const char* filename = rest[0].c_str();
    if (std::FILE* fp = std::fopen(filename, "r")) {
      char buf[1024];
      while (std::size_t len = std::fread(buf, 1, sizeof(buf), fp)) {
        str.append(buf, buf+len);
      }
      std::fclose(fp);
    } else {
      std::string err("iv can't open \"");
      err.append(filename);
      err.append("\"");
      std::perror(err.c_str());
      return EXIT_FAILURE;
    }

    iv::core::Source src(str);
    iv::core::Parser parser(src);
    iv::core::FunctionLiteral* global = parser.ParseProgram();

    if (!global) {
      std::perror(parser.error().c_str());
      return EXIT_FAILURE;
    }

    if (cmd.Exist("ast")) {
      iv::core::AstSerializer ser;
      global->Accept(&ser);
      std::cout << ser << std::endl;
    } else {
      iv::core::CodeGenerator gen;
      global->Accept(&gen);
      std::cout << gen << std::endl;
    }
  } else {
    std::cout << cmd.usage();
    return EXIT_FAILURE;
  }
  return 0;
}
