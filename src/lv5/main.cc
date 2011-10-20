#include <cstdio>
#include <cstdlib>
#include <locale>
#include <string>
#include <vector>
#include <algorithm>
#include <algorithm>
#include <gc/gc.h>
#include "detail/array.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "about.h"
#include "utils.h"
#include "cmdline.h"
#include "ast.h"
#include "ast_serializer.h"
#include "parser.h"
#include "file_source.h"
#include "lv5/factory.h"
#include "lv5/context.h"
#include "lv5/specialized_ast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/command.h"
#include "lv5/fpu.h"
#include "lv5/program.h"
#include "lv5/railgun/railgun.h"
#include "lv5/railgun/command.h"
#include "lv5/railgun/interactive.h"
#include "lv5/teleporter/teleporter.h"
#include "lv5/teleporter/interactive.h"

namespace {

template<typename Source>
iv::lv5::railgun::Code* Compile(iv::lv5::railgun::Context* ctx,
                                const Source& src) {
  iv::lv5::AstFactory factory(ctx);
  iv::core::Parser<iv::lv5::AstFactory, Source> parser(&factory, src);
  const iv::lv5::FunctionLiteral* const global = parser.ParseProgram();
  iv::lv5::railgun::JSScript* script =
      iv::lv5::railgun::JSGlobalScript::New(ctx, &src);
  if (!global) {
    std::fprintf(stderr, "%s\n", parser.error().c_str());
    return NULL;
  }
  return iv::lv5::railgun::Compile(ctx, *global, script);
}

int Execute(const iv::core::StringPiece& data,
            const std::string& filename) {
  iv::lv5::Error e;
  iv::lv5::railgun::Context ctx;
  iv::core::FileSource src(data, filename);
  iv::lv5::railgun::Code* code = Compile(&ctx, src);
  if (!code) {
    return EXIT_FAILURE;
  }
  ctx.DefineFunction<&iv::lv5::Print, 1>("print");
  ctx.DefineFunction<&iv::lv5::Quit, 1>("quit");
  ctx.DefineFunction<&iv::lv5::CollectGarbage, 0>("gc");
  ctx.DefineFunction<&iv::lv5::railgun::Run, 0>("run");
  ctx.DefineFunction<&iv::lv5::HiResTime, 0>("HiResTime");
  ctx.DefineFunction<&iv::lv5::railgun::StackDepth, 0>("StackDepth");
  ctx.vm()->Run(code, &e);
  if (e) {
    const iv::lv5::JSVal res = iv::lv5::JSError::Detail(&ctx, &e);
    e.Clear();
    const iv::lv5::JSString* const str = res.ToString(&ctx, &e);
    if (!e) {
      std::fprintf(stderr, "%s\n", str->GetUTF8().c_str());
      return EXIT_FAILURE;
    } else {
      return EXIT_FAILURE;
    }
  } else {
    return EXIT_SUCCESS;
  }
}

int DisAssemble(const iv::core::StringPiece& data,
                const std::string& filename) {
  iv::lv5::railgun::Context ctx;
  iv::core::FileSource src(data, filename);
  iv::lv5::railgun::Code* code = Compile(&ctx, src);
  if (!code) {
    return EXIT_FAILURE;
  }
  iv::lv5::railgun::OutputDisAssembler dis(&ctx, stdout);
  dis.DisAssemble(*code);
  return EXIT_SUCCESS;
}

int Interpret(const iv::core::StringPiece& data, const std::string& filename) {
  iv::core::FileSource src(data, filename);
  iv::lv5::teleporter::Context ctx;
  iv::lv5::AstFactory factory(&ctx);
  iv::core::Parser<iv::lv5::AstFactory,
                   iv::core::FileSource> parser(&factory, src);
  const iv::lv5::FunctionLiteral* const global = parser.ParseProgram();

  if (!global) {
    std::fprintf(stderr, "%s\n", parser.error().c_str());
    return EXIT_FAILURE;
  }
  ctx.DefineFunction<&iv::lv5::Print, 1>("print");
  ctx.DefineFunction<&iv::lv5::Quit, 1>("quit");
  ctx.DefineFunction<&iv::lv5::HiResTime, 0>("HiResTime");
  iv::lv5::teleporter::JSScript* const script =
      iv::lv5::teleporter::JSGlobalScript::New(&ctx, global, &factory, &src);
  if (ctx.Run(script)) {
    const iv::lv5::JSVal e = ctx.ErrorVal();
    ctx.error()->Clear();
    ctx.SetStatement(iv::lv5::teleporter::Context::NORMAL,
                     iv::lv5::JSEmpty, NULL);
    const iv::lv5::JSString* const str = e.ToString(&ctx, ctx.error());
    if (!*ctx.error()) {
      std::fprintf(stderr, "%s\n", str->GetUTF8().c_str());
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int Ast(const iv::core::StringPiece& data, const std::string& filename) {
  iv::core::FileSource src(data, filename);
  iv::lv5::Context ctx;
  iv::lv5::AstFactory factory(&ctx);
  iv::core::Parser<iv::lv5::AstFactory,
                   iv::core::FileSource> parser(&factory, src);
  const iv::lv5::FunctionLiteral* const global = parser.ParseProgram();

  if (!global) {
    std::fprintf(stderr, "%s\n", parser.error().c_str());
    return EXIT_FAILURE;
  }
  iv::core::ast::AstSerializer<iv::lv5::AstFactory> ser;
  ser.Visit(global);
  const iv::core::UString str = ser.out();
  iv::core::unicode::FPutsUTF16(stdout, str.begin(), str.end());
  return EXIT_SUCCESS;
}

}  // namespace anonymous

int main(int argc, char **argv) {
  iv::lv5::FPU fpu;
  iv::lv5::program::Init(argc, argv);
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
  cmd.Add<std::string>(
      "execute",
      "execute",
      'e', "execute command", false);
  cmd.Add("ast",
          "ast",
          0, "print ast");
  cmd.Add("interp",
          "interp",
          0, "use interpreter");
  cmd.Add("dis",
          "dis",
          0, "print bytecode");
  cmd.Add("copyright",
          "copyright",
          0,   "print the copyright");
  cmd.set_footer("[program_file] [arguments]");

  const bool cmd_parse_success = cmd.Parse(argc, argv);
  if (!cmd_parse_success) {
    std::fprintf(stderr, "%s\n%s",
                 cmd.error().c_str(), cmd.usage().c_str());
    return EXIT_FAILURE;
  }

  if (cmd.Exist("help")) {
    std::fputs(cmd.usage().c_str(), stdout);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("version")) {
    std::printf("lv5 %s (compiled %s %s)\n", IV_VERSION, __DATE__, __TIME__);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("copyright")) {
    std::printf("lv5 - Copyright (C) 2010 %s\n", IV_DEVELOPER);
    return EXIT_SUCCESS;
  }

  const std::vector<std::string>& rest = cmd.rest();
  if (!rest.empty() || cmd.Exist("file") || cmd.Exist("execute")) {
    std::vector<char> res;
    std::string filename;
    if (cmd.Exist("file")) {
      const std::vector<std::string>& vec = cmd.GetList<std::string>("file");
      for (std::vector<std::string>::const_iterator it = vec.begin(),
           last = vec.end(); it != last; ++it, filename.push_back(' ')) {
        filename.append(*it);
        if (!iv::core::ReadFile(*it, &res)) {
          return EXIT_FAILURE;
        }
      }
    } else if (cmd.Exist("execute")) {
      const std::string& com = cmd.Get<std::string>("execute");
      filename = "<command>";
      res.insert(res.end(), com.begin(), com.end());
    } else {
      filename = rest.front();
      if (!iv::core::ReadFile(filename, &res)) {
        return EXIT_FAILURE;
      }
    }
    const iv::core::StringPiece src(res.data(), res.size());
    if (cmd.Exist("ast")) {
      return Ast(src, filename);
    } else if (cmd.Exist("dis")) {
      return DisAssemble(src, filename);
    } else if (cmd.Exist("interp")) {
      return Interpret(src, filename);
    } else {
      return Execute(src, filename);
    }
  } else {
    // Interactive Shell Mode
    if (cmd.Exist("interp")) {
      iv::lv5::teleporter::Interactive shell;
      return shell.Run();
    } else {
      iv::lv5::railgun::Interactive shell;
      return shell.Run();
    }
  }
  return 0;
}
