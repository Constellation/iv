#ifndef IV_LV5_INTERACTIVE_H_
#define IV_LV5_INTERACTIVE_H_
#include <cstdlib>
#include <cstdio>
#include <iostream>  // NOLINT
#include <tr1/memory>
#include <tr1/array>
#include "none.h"
#include "token.h"
#include "parser.h"
#include "context.h"
#include "factory.h"
#include "stringpiece.h"
#include "jsscript.h"
#include "jsval.h"
#include "jsstring.h"
#include "icu/source.h"
#include "icu/ustream.h"
namespace iv {
namespace lv5 {
namespace detail {
template<typename T>
class InteractiveData {
 public:
  static const std::string kOrigin;
};
template<typename T>
const std::string InteractiveData<T>::kOrigin = "(shell)";
}

typedef detail::InteractiveData<core::None> InteractiveData;

class Interactive {
 public:
  Interactive()
    : ctx_() {
  }
  int Run() {
    std::string buffer;
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
      buffer.append(line.data());
      JSEvalScript<icu::Source>* script = Parse(buffer, &recover);
      if (script) {
        buffer.clear();
        JSVal val;
        if (ctx_.Run(script)) {
          val = ctx_.ErrorVal();
          ctx_.error()->Clear();
        } else {
          val = ctx_.ret();
        }
        const JSString* const str = val.ToString(&ctx_, ctx_.error());
        if (!ctx_.IsError()) {
          std::cout << str->data() << std::endl;
        } else {
          std::cout << "FATAL ERROR" << std::endl;
          break;
        }
      } else if (!recover) {
        buffer.clear();
      }
    }
    return EXIT_SUCCESS;
  }
 private:
  JSEvalScript<icu::Source>* Parse(const std::string& text, bool* recover) {
    std::tr1::shared_ptr<icu::Source> src(
        new icu::Source(text, InteractiveData::kOrigin));
    AstFactory* const factory = new AstFactory(&ctx_);
    core::Parser<AstFactory, icu::Source> parser(factory, src.get());
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
      return JSEvalScript<icu::Source>::New(&ctx_, eval, factory, src);
    }
  }
  Context ctx_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INTERACTIVE_H_
