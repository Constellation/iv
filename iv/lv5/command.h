#ifndef IV_LV5_COMMAND_H_
#define IV_LV5_COMMAND_H_
#include <cstdio>
#include <cstdlib>
extern "C" {
#include <gc/gc.h>
}
#include <iv/conversions.h>
#include <iv/unicode.h>
#include <iv/date_utils.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/arguments.h>
namespace iv {
namespace lv5 {

inline JSVal Print(const Arguments& args, Error* e) {
  if (!args.empty()) {
    Context* const ctx = args.ctx();
    for (Arguments::const_iterator it = args.begin(),
         last = args.end(); it != last;) {
      JSString* const str = it->ToString(ctx, IV_LV5_ERROR(e));
      std::cout << *str;
      if (++it != last) {
        std::fputc(' ', stdout);
      } else {
        std::fputc('\n', stdout);
      }
    }
    std::fflush(stdout);
  }
  return JSUndefined;
}

inline JSVal Log(const Arguments& args, Error* e) {
  if (!args.empty()) {
    Context* const ctx = args.ctx();
    JSString* const str = args.front().ToString(ctx, IV_LV5_ERROR(e));
    std::cout << *str;
    std::fflush(stdout);
  }
  return JSUndefined;
}


inline JSVal Quit(const Arguments& args, Error* e) {
  const int32_t code = (args.empty()) ?
      0 : args.front().ToInt32(args.ctx(), IV_LV5_ERROR(e));
  std::exit(code);
  return JSUndefined;
}

inline JSVal HiResTime(const Arguments& args, Error* e) {
  return core::date::HighResTime();
}

inline JSVal CollectGarbage(const Arguments& args, Error* e) {
  GC_gcollect();
  return JSUndefined;
}

} }  // namespace iv::lv5
#endif  // IV_LV5_COMMAND_H_
