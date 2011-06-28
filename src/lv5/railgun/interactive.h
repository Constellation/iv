#ifndef _IV_LV5_RAILGUN_INTERACTIVE_H_
#define _IV_LV5_RAILGUN_INTERACTIVE_H_
#include <cstdlib>
#include <cstdio>
#include "detail/memory.h"
#include "detail/array.h"
#include "token.h"
#include "parser.h"
#include "unicode.h"
#include "stringpiece.h"
#include "lv5/context.h"
#include "lv5/factory.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/command.h"
#include "lv5/railgun.h"
namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

static const std::string kInteractiveOrigin = "(shell)";

}  // namespace detail


class Interactive {
 public:
  Interactive()
    : vm_(),
      ctx_(&vm_) {
    vm_.set_context(&ctx_);
    ctx_.DefineFunction<&lv5::Print, 1>("print");
    ctx_.DefineFunction<&lv5::Quit, 1>("quit");
    ctx_.DefineFunction<&iv::lv5::HiResTime, 0>("HiResTime");
  }

  int Run() {
    Error e;
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
      buffer.insert(buffer.end(), line.data(), line.data() + std::strlen(line.data()));
      Code* code = Parse(core::StringPiece(buffer.data(), buffer.size()), &recover);
      if (code) {
        buffer.clear();
        const std::pair<JSVal, VM::Status> pair = vm_.Run(code, &e);
        JSVal ret;
        if (e) {
          ret = iv::lv5::JSError::Detail(&ctx_, &e);
          e.Clear();
        } else {
          ret = pair.first;
        }
        if (!ret.IsUndefined()) {
          const JSString* const str = ret.ToString(&ctx_, &e);
          if (!e) {
            core::unicode::FPutsUTF16(stdout, str->begin(), str->end());
            std::fputc('\n', stdout);
          } else {
            ret = iv::lv5::JSError::Detail(&ctx_, &e);
            e.Clear();
            const JSString* const str = ret.ToString(&ctx_, &e);
            if (!e) {
              core::unicode::FPutsUTF16(stdout, str->begin(), str->end());
              std::fputc('\n', stdout);
            } else {
              e.Clear();
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
  Code* Parse(const core::StringPiece& text, bool* recover) {
    std::shared_ptr<core::FileSource> const src(
        new core::FileSource(text, detail::kInteractiveOrigin));
    AstFactory factory(&ctx_);
    core::Parser<AstFactory, core::FileSource> parser(&factory, *src);
    const FunctionLiteral* const eval = parser.ParseProgram();
    if (!eval) {
      if (parser.RecoverableError()) {
        *recover = true;
      } else {
        std::fprintf(stderr, "%s\n", parser.error().c_str());
      }
      return NULL;
    }
    JSScript* script = JSEvalScript<core::FileSource>::New(&ctx_, src);
    return Compile(&ctx_, *eval, script);
  }

  VM vm_;
  Context ctx_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_INTERACTIVE_H_
