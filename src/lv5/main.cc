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
#include <signal.h>
#include "config/config.h"

#include "stringpiece.h"
#include "ustringpiece.h"

#include "cmdline.h"
#include "ast.h"

#include "ast_serializer.h"

#include "parser.h"
#include "factory.h"

#include "interpreter.h"
#include "context.h"
#include "jsast.h"
#include "jsval.h"
#include "jsstring.h"
#include "jsscript.h"
#include "command.h"
#include "interactive.h"

#include "icu/ustream.h"
#include "icu/source.h"

#include "fpu.h"

static void SegvHandler(int sn, siginfo_t* si,
                        void* sc) {
  std::cout << "SIG SEGV!!" << std::endl;
  std::cerr << "SIG SEGV!!" << std::endl;
  std::exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
  using iv::lv5::JSVal;
  iv::lv5::FixFPU();
  GC_INIT();

  struct sigaction sig;
  sig.sa_flags = SA_SIGINFO;
  sig.sa_sigaction = &SegvHandler;
  sigaction(SIGSEGV, &sig, reinterpret_cast<struct sigaction*>(NULL));

  iv::cmdline::Parser cmd("lv5");

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
  const std::string& filename = rest.front();
  if (!rest.empty()) {
    std::string str;
    if (std::FILE* fp = std::fopen(filename.c_str(), "r")) {
      std::tr1::array<char, 1024> buf;
      while (const std::size_t len = std::fread(
              buf.data(),
              1,
              buf.size(), fp)) {
        str.append(buf.data(), len);
      }
      std::fclose(fp);
    } else {
      std::string err("lv5 can't open \"");
      err.append(filename);
      err.append("\"");
      std::perror(err.c_str());
      return EXIT_FAILURE;
    }

    iv::icu::Source src(str, filename);
    iv::lv5::Context ctx;
    iv::lv5::AstFactory factory(&ctx);
    iv::core::Parser<iv::lv5::AstFactory, iv::icu::Source>
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
      ctx.DefineFunction(&iv::lv5::Print, "print", 1);
      ctx.DefineFunction(&iv::lv5::Quit, "quit", 1);
      iv::lv5::JSScript* const script = iv::lv5::JSGlobalScript::New(
          &ctx, global, &factory, &src);
      if (ctx.Run(script)) {
        const JSVal e = ctx.ErrorVal();
        ctx.error()->Clear();
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
