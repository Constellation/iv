#include <cstdio>
#include <cstdlib>
#include <locale>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>  // NOLINT
#include <algorithm>
#include <tr1/array>
#include <unicode/ustdio.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>

#include "stringpiece.h"
#include "ustringpiece.h"

#include "config.h"
#include "cmdline.h"
#include "ast.h"
#include "ast-serializer.h"
#include "parser.h"
#include "factory-icu.h"

#include "interpreter.h"
#include "context.h"
#include "jsval.h"
#include "error.h"
#include "arguments.h"

#include "ustream.h"

namespace {
const char* const PHONIC_VERSION = "0.0.1";
}  // namespace

int main(int argc, char **argv) {
  std::locale::global(std::locale(""));

  iv::cmdline::Parser cmd("phonic");

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
    printf("phonic %s (compiled %s %s)\n", PHONIC_VERSION, __DATE__, __TIME__);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("copyright")) {
    printf("phonic - Copyright (C) 2010 %s\n", IV_DEVELOPER);
    return EXIT_SUCCESS;
  }

  const std::vector<std::string>& rest = cmd.rest();
  const std::string& filename = rest.front();
  if (!rest.empty()) {
    std::string str;
    if (std::FILE* fp = std::fopen(filename.c_str(), "r")) {
      std::tr1::array<char, 1024> buf;
      while (std::size_t len = std::fread(buf.data(),
                                          1,
                                          buf.size(), fp)) {
        str.append(buf.data(), len);
      }
      std::fclose(fp);
    } else {
      std::string err("phonic can't open \"");
      err.append(filename);
      err.append("\"");
      std::perror(err.c_str());
      return EXIT_FAILURE;
    }

    iv::core::Source src(str, filename);
    iv::phonic::AstFactory factory;
    iv::core::Parser parser(&src, &factory);
    iv::core::FunctionLiteral* global = parser.ParseProgram();

    if (!global) {
      std::cerr << parser.error() << std::endl;
      return EXIT_FAILURE;
    }

    if (cmd.Exist("ast")) {
      iv::core::AstSerializer ser;
      global->Accept(&ser);
      std::cout << ser.out().data() << std::endl;
    } else {
      iv::lv5::Context ctx;
      ctx.DefineFunction(&Print, "print", 1);
      if (ctx.Run(global)) {
        const JSVal e = ctx.ErrorVal();
        ctx.error()->Clear();
        const JSString* const str = e.ToString(&ctx, ctx.error());
        if (!*ctx.error()) {
          std::cout << str->data() << std::endl;
        }
      }
    }
  } else {
    std::cout << cmd.usage();
    return EXIT_FAILURE;
  }
  return 0;
}
