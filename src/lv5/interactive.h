#ifndef _IV_LV5_INTERACTIVE_H_
#define _IV_LV5_INTERACTIVE_H_
#include <cstdlib>
#include <cstdio>
#include <tr1/memory>
#include <tr1/array>
#include "token.h"
#include "parser.h"
#include "unicode.h"
#include "stringpiece.h"
#include "lv5/context.h"
#include "lv5/factory.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/command.h"
#include "lv5/teleporter.h"
namespace iv {
namespace lv5 {
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
      std::tr1::array<char, 1024> line;
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
      buffer.insert(buffer.end(), line.data(), line.data() + std::strlen(line.data()));
      teleporter::JSEvalScript<core::FileSource>* script =
          Parse(core::StringPiece(buffer.data(), buffer.size()), &recover);
      if (script) {
        buffer.clear();
        JSVal val;
        if (ctx_.Run(script)) {
          val = ctx_.ErrorVal();
          ctx_.error()->Clear();
          ctx_.SetStatement(teleporter::Context::NORMAL, JSEmpty, NULL);
        } else {
          val = ctx_.ret();
        }
        if (!val.IsUndefined()) {
          const JSString* const str = val.ToString(&ctx_, ctx_.error());
          if (!ctx_.IsError()) {
            core::unicode::FPutsUTF16(stdout, str->begin(), str->end());
            std::fputc('\n', stdout);
          } else {
            val = ctx_.ErrorVal();
            ctx_.error()->Clear();
            ctx_.SetStatement(teleporter::Context::NORMAL, JSEmpty, NULL);
            const JSString* const str = val.ToString(&ctx_, ctx_.error());
            if (!ctx_.IsError()) {
              core::unicode::FPutsUTF16(stdout, str->begin(), str->end());
              std::fputc('\n', stdout);
            } else {
              ctx_.error()->Clear();
              ctx_.SetStatement(teleporter::Context::NORMAL, JSEmpty, NULL);
              std::puts("<STRING CONVERSION FAILED>\n");
            }
          }
        }
      } else if (!recover) {
        buffer.clear();
      }
    }
    return EXIT_SUCCESS;
  }
 private:
  teleporter::JSEvalScript<core::FileSource>* Parse(const core::StringPiece& text,
                                               bool* recover) {
    std::tr1::shared_ptr<core::FileSource> src(
        new core::FileSource(text, detail::kInteractiveOrigin));
    AstFactory* const factory = new AstFactory(&ctx_);
    core::Parser<AstFactory, core::FileSource> parser(factory, *src);
    parser.set_strict(ctx_.IsStrict());
    const FunctionLiteral* const eval = parser.ParseProgram();
    if (!eval) {
      if (parser.RecoverableError()) {
        *recover = true;
      } else {
        std::cerr << parser.error() << std::endl;
      }
      delete factory;
      return NULL;
    } else {
      return teleporter::JSEvalScript<core::FileSource>::New(&ctx_, eval,
                                                             factory, src);
    }
  }
  teleporter::Context ctx_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_INTERACTIVE_H_
