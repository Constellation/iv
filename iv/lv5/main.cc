#include <cstdio>
#include <cstdlib>
#include <locale>
#include <string>
#include <vector>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/about.h>
#include <iv/cmdline.h>
#include <iv/lv5/lv5.h>
#include <iv/lv5/teleporter/interactive.h>
#include <iv/lv5/railgun/command.h>
#include <iv/lv5/railgun/interactive.h>
#include <iv/lv5/breaker/command.h>
#include <iv/lv5/melt/melt.h>
#if defined(IV_OS_MACOSX) || defined(IV_OS_LINUX) || defined(IV_OS_BSD)
#include <signal.h>
#endif

namespace {

void InitContext(iv::lv5::Context* ctx) {
  iv::lv5::Error::Dummy dummy;
  ctx->DefineFunction<&iv::lv5::Print, 1>("print");
  ctx->DefineFunction<&iv::lv5::Log, 1>("log");  // this is simply output log function
  ctx->DefineFunction<&iv::lv5::Quit, 1>("quit");
  ctx->DefineFunction<&iv::lv5::CollectGarbage, 0>("gc");
  ctx->DefineFunction<&iv::lv5::HiResTime, 0>("HiResTime");
  ctx->DefineFunction<&iv::lv5::railgun::Dis, 1>("dis");
  iv::lv5::melt::Console::Export(ctx, &dummy);
}

#if defined(IV_ENABLE_JIT)
int BreakerExecute(const iv::core::StringPiece& data,
                   const std::string& filename, bool statistics) {
  iv::lv5::Error::Standard e;
  iv::lv5::breaker::Context ctx;
  InitContext(&ctx);
  ctx.DefineFunction<&iv::lv5::breaker::Run, 1>("run");
  ctx.DefineFunction<&iv::lv5::breaker::Load, 1>("load");
  std::shared_ptr<iv::core::FileSource>
      src(new iv::core::FileSource(data, filename));
  iv::lv5::breaker::ExecuteInGlobal(&ctx, src, &e);
  if (e) {
    e.Dump(&ctx, stderr);
    return EXIT_FAILURE;
  }
  ctx.Validate();
  return EXIT_SUCCESS;
}

int BreakerExecuteFiles(const std::vector<std::string>& filenames) {
  iv::lv5::Error::Standard e;
  iv::lv5::breaker::Context ctx;
  InitContext(&ctx);
  ctx.DefineFunction<&iv::lv5::breaker::Run, 1>("run");
  ctx.DefineFunction<&iv::lv5::breaker::Load, 1>("load");

  std::vector<char> res;
  for (std::vector<std::string>::const_iterator it = filenames.begin(),
       last = filenames.end(); it != last; ++it) {
    if (!iv::core::ReadFile(*it, &res)) {
      return EXIT_FAILURE;
    }
    std::shared_ptr<iv::core::FileSource>
        src(new iv::core::FileSource(
                iv::core::StringPiece(res.data(), res.size()), *it));
    res.clear();
    iv::lv5::breaker::ExecuteInGlobal(&ctx, src, &e);
    if (e) {
      e.Dump(&ctx, stderr);
      return EXIT_FAILURE;
    }
  }
  ctx.Validate();
  return EXIT_SUCCESS;
}
#endif

int RailgunExecute(const iv::core::StringPiece& data,
                   const std::string& filename, bool statistics) {
  iv::lv5::Error::Standard e;
  iv::lv5::railgun::Context ctx;
  InitContext(&ctx);
  ctx.DefineFunction<&iv::lv5::railgun::Run, 0>("run");

  std::shared_ptr<iv::core::FileSource>
      src(new iv::core::FileSource(data, filename));
  iv::lv5::railgun::ExecuteInGlobal(&ctx, src, &e);
  if (e) {
    e.Dump(&ctx, stderr);
    return EXIT_FAILURE;
  }
  if (statistics) {
    ctx.vm()->DumpStatistics();
  }
  ctx.Validate();
  return EXIT_SUCCESS;
}

int RailgunExecuteFiles(const std::vector<std::string>& filenames) {
  iv::lv5::Error::Standard e;
  iv::lv5::railgun::Context ctx;
  InitContext(&ctx);
  ctx.DefineFunction<&iv::lv5::railgun::Run, 0>("run");

  std::vector<char> res;
  for (std::vector<std::string>::const_iterator it = filenames.begin(),
       last = filenames.end(); it != last; ++it) {
    if (!iv::core::ReadFile(*it, &res)) {
      return EXIT_FAILURE;
    }
    std::shared_ptr<iv::core::FileSource>
        src(new iv::core::FileSource(
                iv::core::StringPiece(res.data(), res.size()), *it));
    res.clear();
    iv::lv5::railgun::ExecuteInGlobal(&ctx, src, &e);
    if (e) {
      e.Dump(&ctx, stderr);
      return EXIT_FAILURE;
    }
  }
  ctx.Validate();
  return EXIT_SUCCESS;
}

int DisAssemble(const iv::core::StringPiece& data,
                const std::string& filename) {
  iv::lv5::railgun::Context ctx;
  iv::lv5::Error::Standard e;
  std::shared_ptr<iv::core::FileSource>
      src(new iv::core::FileSource(data, filename));
  iv::lv5::railgun::Code* code =
      iv::lv5::railgun::CompileInGlobal(&ctx, src, true, &e);
  if (e) {
    return EXIT_FAILURE;
  }
  iv::lv5::railgun::OutputDisAssembler dis(&ctx, stdout);
  dis.DisAssembleGlobal(*code);
  return EXIT_SUCCESS;
}

int Interpret(const iv::core::StringPiece& data, const std::string& filename) {
  iv::core::FileSource src(data, filename);
  iv::lv5::teleporter::Context ctx;
  iv::lv5::AstFactory factory(&ctx);
  iv::core::Parser<iv::lv5::AstFactory,
                   iv::core::FileSource> parser(&factory,
                                                src, ctx.symbol_table());
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
    ctx.error()->Dump(&ctx, stderr);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int Ast(const iv::core::StringPiece& data, const std::string& filename) {
  iv::core::FileSource src(data, filename);
  iv::lv5::railgun::Context ctx;
  iv::lv5::AstFactory factory(&ctx);
  iv::core::Parser<iv::lv5::AstFactory,
                   iv::core::FileSource> parser(&factory,
                                                src, ctx.symbol_table());
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
  iv::lv5::Init();

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
  cmd.Add("signal",
          "",
          's', "install signal handlers");
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
  cmd.Add("railgun",
          "railgun",
          0, "force railgun VM");
  cmd.Add("dis",
          "dis",
          'd', "print bytecode");
  cmd.Add("statistics",
          "statistics",
          0, "print statistics");
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
    const char* jit
#if defined(IV_ENABLE_JIT)
        = "on";
#else
        = "off";
#endif
    std::printf("lv5 %s (compiled %s %s with JIT [%s])\n", IV_VERSION, __DATE__, __TIME__, jit);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("copyright")) {
    std::printf("lv5 - %s\n", IV_COPYRIGHT);
    return EXIT_SUCCESS;
  }

  if (cmd.Exist("signal")) {
#if defined(IV_OS_MACOSX) || defined(IV_OS_LINUX) || defined(IV_OS_BSD)
    signal(SIGILL, _exit);
    signal(SIGFPE, _exit);
    signal(SIGBUS, _exit);
    signal(SIGSEGV, _exit);
#endif
  }

  const std::vector<std::string>& rest = cmd.rest();
  if (!rest.empty() || cmd.Exist("file") || cmd.Exist("execute")) {
    std::vector<char> res;
    std::string filename;
    if (cmd.Exist("file")) {
      const std::vector<std::string>& vec = cmd.GetList<std::string>("file");
      if (!cmd.Exist("ast") && !cmd.Exist("dis") && !cmd.Exist("interp")) {
        if (cmd.Exist("railgun")) {
          return RailgunExecuteFiles(vec);
        }
#if defined(IV_ENABLE_JIT)
        return BreakerExecuteFiles(vec);
#else
        return RailgunExecuteFiles(vec);
#endif
      }
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
    } else if (cmd.Exist("railgun")) {
      return RailgunExecute(src, filename, cmd.Exist("statistics"));
    } else {
#if defined(IV_ENABLE_JIT)
      return BreakerExecute(src, filename, cmd.Exist("statistics"));
#else
      return RailgunExecute(src, filename, cmd.Exist("statistics"));
#endif
    }
  } else {
    // Interactive Shell Mode
    if (cmd.Exist("interp")) {
      iv::lv5::teleporter::Interactive shell;
      return shell.Run();
    } else {
      iv::lv5::railgun::Interactive shell(cmd.Exist("dis"));
      return shell.Run();
    }
  }
  return 0;
}
