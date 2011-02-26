#include <cstdio>
#include <cstdlib>
#include <locale>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>  // NOLINT
#include <algorithm>
#include <tr1/array>
#include <gc/gc.h>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "about.h"
#include "cmdline.h"
#include "ast.h"
#include "ast_serializer.h"
#include "parser.h"
#include "icu/ustream.h"
#include "icu/source.h"
#include "lv5/factory.h"
#include "lv5/interpreter.h"
#include "lv5/context.h"
#include "lv5/jsast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/jsscript.h"
#include "lv5/command.h"
#include "lv5/interactive.h"
#include "lv5/fpu.h"

namespace {

bool ReadFile(const std::string& filename, std::vector<char>* out) {
  if (std::FILE* fp = std::fopen(filename.c_str(), "r")) {
    std::tr1::array<char, 1024> buf;
    while (const std::size_t len = std::fread(
            buf.data(),
            1,
            buf.size(), fp)) {
      out->insert(out->end(), buf.begin(), buf.begin() + len);
    }
    std::fclose(fp);
    return true;
  } else {
    std::string err("lv5 can't open \"");
    err.append(filename);
    err.append("\"");
    std::perror(err.c_str());
    return false;
  }
}

}  // namespace

int main(int argc, char **argv) {
  using iv::lv5::JSVal;
  iv::lv5::FPU fpu;
  GC_INIT();

  iv::cmdline::Parser cmd("lv5");

  cmd.Add("help",
          "help",
          'h', "print this message");
  cmd.Add("version",
          "version",
          'v', "print the version");
  cmd.AddList<std::string>(
      "file",
      "file",
      'f', "script file to load");
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

  const bool cmd_parse_success = cmd.Parse(argc, argv);
  if (!cmd_parse_success) {
    std::cerr << cmd.error() << std::endl << cmd.usage();
    return EXIT_FAILURE;
  }

  if (cmd.Exist("help")) {
    std::cout << cmd.usage();
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("version")) {
    printf("lv5 %s (compiled %s %s)\n", IV_VERSION, __DATE__, __TIME__);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("copyright")) {
    printf("lv5 - Copyright (C) 2010 %s\n", IV_DEVELOPER);
    return EXIT_SUCCESS;
  }

  const std::vector<std::string>& rest = cmd.rest();
  if (!rest.empty() || cmd.Exist("file")) {
    std::vector<char> res;
    std::string filename;
    if (cmd.Exist("file")) {
      const std::vector<std::string>& vec = cmd.GetList<std::string>("file");
      for (std::vector<std::string>::const_iterator it = vec.begin(),
           last = vec.end(); it != last; ++it, filename.push_back(' ')) {
        filename.append(*it);
        if (!ReadFile(*it, &res)) {
          return EXIT_FAILURE;
        }
      }
    } else {
      filename = rest.front();
      if (!ReadFile(filename, &res)) {
        return EXIT_FAILURE;
      }
    }

    iv::icu::Source src(iv::core::StringPiece(res.data(), res.size()), filename);
    iv::lv5::Context ctx;
    iv::lv5::AstFactory factory(&ctx);
    iv::core::Parser<iv::lv5::AstFactory, iv::icu::Source, true, true>
        parser(&factory, &src);
    const iv::lv5::FunctionLiteral* const global = parser.ParseProgram();

    if (!global) {
      std::cerr << parser.error() << std::endl;
      return EXIT_FAILURE;
    }

    if (cmd.Exist("ast")) {
      iv::core::ast::AstSerializer<iv::lv5::AstFactory> ser;
      global->Accept(&ser);
      std::cout << ser.out() << std::endl;
    } else {
      ctx.DefineFunction<&iv::lv5::Print, 1>("print");
      ctx.DefineFunction<&iv::lv5::Quit, 1>("quit");
      iv::lv5::JSScript* const script = iv::lv5::JSGlobalScript::New(
          &ctx, global, &factory, &src);
      if (ctx.Run(script)) {
        const JSVal e = ctx.ErrorVal();
        ctx.error()->Clear();
        ctx.SetStatement(iv::lv5::Context::Context::NORMAL,
                         iv::lv5::JSEmpty, NULL);
        const iv::lv5::JSString* const str = e.ToString(&ctx, ctx.error());
        if (!*ctx.error()) {
          std::cerr << *str << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  } else {
    // Interactive Shell Mode
    iv::lv5::Interactive shell;
    return shell.Run();
  }
  return 0;
}
