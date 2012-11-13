#ifndef IV_LV5_MELT_MELT_CONSOLE_CONSOLE_H_
#define IV_LV5_MELT_MELT_CONSOLE_CONSOLE_H_
#include <cstdio>
#include <iv/lv5/lv5.h>
namespace iv {
namespace lv5 {
namespace melt {

class Console {
 public:
  static JSVal Log(const Arguments& args, Error* e) {
    if (!args.empty()) {
      Context* const ctx = args.ctx();
      for (Arguments::const_iterator it = args.begin(),
           last = args.end(); it != last;) {
        JSString* const str = it->ToString(ctx, IV_LV5_ERROR(e));
        std::cout << *str;
        if (++it != last) {
          std::fputc(' ', stdout);
        }
      }
      std::fputc('\n', stdout);
    }
    std::fflush(stdout);
    return JSUndefined;
  }

  static void Export(Context* ctx, Error* e) {
    JSObject* obj = JSObject::New(ctx);
    const Symbol name = ctx->Intern("log");
    obj->DefineOwnProperty(
        ctx, name,
        DataDescriptor(
          JSInlinedFunction<&Log, 1>::New(ctx, name),
          ATTR::W | ATTR::C),
      false, e);
    ctx->Import("console", obj);
  }
};

} } }  // namespace iv::lv5::melt
#endif  // IV_LV5_MELT_MELT_CONSOLE_CONSOLE_H_
