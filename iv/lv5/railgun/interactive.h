#ifndef IV_LV5_RAILGUN_INTERACTIVE_H_
#define IV_LV5_RAILGUN_INTERACTIVE_H_
#include <cstdlib>
#include <cstdio>
#include <iv/detail/memory.h>
#include <iv/detail/array.h>
#include <iv/token.h>
#include <iv/parser.h>
#include <iv/unicode.h>
#include <iv/stringpiece.h>
#include <iv/lv5/context.h>
#include <iv/lv5/factory.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/command.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/command.h>
#include <iv/lv5/melt/melt.h>
namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

static const std::string kInteractiveOrigin = "(shell)";

}  // namespace detail


class Interactive {
 public:
  explicit Interactive(bool disassemble)
    : ctx_(),
      disassemble_(disassemble) {
    Error::Dummy dummy;
    ctx_.DefineFunction<&Print, 1>("print");
    ctx_.DefineFunction<&Quit, 1>("quit");
    ctx_.DefineFunction<&CollectGarbage, 0>("gc");
    ctx_.DefineFunction<&HiResTime, 0>("HiResTime");
    ctx_.DefineFunction<&railgun::Run, 0>("run");
    ctx_.DefineFunction<&railgun::StackDepth, 0>("StackDepth");
    ctx_.DefineFunction<&railgun::Dis, 1>("dis");
    melt::Console::Export(&ctx_, &dummy);
  }

  int Run() {
    Error::Standard e;
    std::vector<char> buffer;
    while (true) {
      std::array<char, 1024> line;
      bool recover = false;
      if (buffer.empty()) {
        std::printf("> ");
      } else {
        std::printf("| ");
      }
      const char* str = std::fgets(line.data(), line.size(), stdin);
      if (!str) {
        break;
      }
      buffer.insert(buffer.end(),
                    line.data(), line.data() + std::strlen(line.data()));
      Code* code = Parse(
          core::StringPiece(buffer.data(), buffer.size()), &recover);
      if (code) {
        buffer.clear();
        if (disassemble_) {
          OutputDisAssembler dis(&ctx_, stdout);
          dis.DisAssemble(*code);
        }
        JSVal ret = ctx_.vm()->Run(code, &e);
        if (e) {
          ret = JSUndefined;
          e.Dump(&ctx_, stderr);
        } else {
          const JSString* const str = ret.ToString(&ctx_, &e);
          if (!e) {
            std::printf("%s\n", str->GetUTF8().c_str());
          } else {
            e.Dump(&ctx_, stderr);
          }
        }
      } else if (!recover) {
        buffer.clear();
      }
    }
    return EXIT_SUCCESS;
  }
 private:
  Code* Parse(const core::StringPiece& text, bool* recover) {
    std::shared_ptr<core::FileSource> const src(
        new core::FileSource(text, detail::kInteractiveOrigin));
    AstFactory factory(&ctx_);
    core::Parser<AstFactory, core::FileSource> parser(&factory,
                                                      *src,
                                                      ctx_.symbol_table());
    const FunctionLiteral* const eval = parser.ParseProgram();
    if (!eval) {
      if (parser.RecoverableError()) {
        *recover = true;
      } else {
        std::fprintf(stderr, "%s\n", parser.error().c_str());
      }
      return nullptr;
    }
    JSScript* script = JSSourceScript<core::FileSource>::New(&ctx_, src);
    return CompileIndirectEval(&ctx_, *eval, script);
  }

  Context ctx_;
  bool disassemble_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_INTERACTIVE_H_
