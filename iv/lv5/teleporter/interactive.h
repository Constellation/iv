#ifndef IV_LV5_TELEPORTER_INTERACTIVE_H_
#define IV_LV5_TELEPORTER_INTERACTIVE_H_
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
#include <iv/lv5/teleporter/teleporter.h>
namespace iv {
namespace lv5 {
namespace teleporter {
namespace detail {

static const std::string kInteractiveOrigin = "(shell)";

}  // namespace detail


class Interactive {
 public:
  Interactive()
    : ctx_() {
    ctx_.DefineFunction<&lv5::Print, 1>("print");
    ctx_.DefineFunction<&lv5::Quit, 1>("quit");
    ctx_.DefineFunction<&iv::lv5::HiResTime, 0>("HiResTime");
  }
  int Run() {
    std::vector<char> buffer;
    while (true) {
      std::array<char, 1024> line;
      bool recover = false;
      if (buffer.empty()) {
        std::printf("> ");
      } else {
        std::printf("| ");
      }
      const char* str =
          std::fgets(line.data(), static_cast<int>(line.size()), stdin);
      if (!str) {
        break;
      }
      buffer.insert(buffer.end(),
                    line.data(), line.data() + std::strlen(line.data()));
      JSEvalScript<core::FileSource>* script =
          Parse(core::StringPiece(buffer.data(), buffer.size()), &recover);
      if (script) {
        buffer.clear();
        JSVal val;
        if (ctx_.Run(script)) {
          val = ctx_.ErrorVal();
          ctx_.SetStatement(Context::NORMAL, JSEmpty, NULL);
        } else {
          val = ctx_.ret();
        }
        if (!val.IsUndefined()) {
          const JSString* const str = val.ToString(&ctx_, ctx_.error());
          if (!ctx_.IsError()) {
            std::printf("%s\n", str->GetUTF8().c_str());
          } else {
            val = JSUndefined;
            ctx_.error()->Dump(&ctx_, stderr);
          }
        }
      } else if (!recover) {
        buffer.clear();
      }
    }
    return EXIT_SUCCESS;
  }
 private:
  JSEvalScript<core::FileSource>* Parse(const core::StringPiece& text,
                                        bool* recover) {
    std::shared_ptr<core::FileSource> src(
        new core::FileSource(text, detail::kInteractiveOrigin));
    AstFactory* const factory = new AstFactory(&ctx_);
    core::Parser<AstFactory, core::FileSource> parser(factory,
                                                      *src,
                                                      ctx_.symbol_table());
    parser.set_strict(ctx_.IsStrict());
    const FunctionLiteral* const eval = parser.ParseProgram();
    if (!eval) {
      if (parser.RecoverableError()) {
        *recover = true;
      } else {
        std::fprintf(stderr, "%s\n", parser.error().c_str());
      }
      delete factory;
      return NULL;
    } else {
      return JSEvalScript<core::FileSource>::New(&ctx_, eval,
                                                 factory, src);
    }
  }
  Context ctx_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // IV_LV5_TELEPORTER_INTERACTIVE_H_
