#ifndef _IV_LV5_COMMAND_H_
#define _IV_LV5_COMMAND_H_
#include <cstdio>
#include <cstdlib>
extern "C" {
#include <gc/gc.h>
}
#include "conversions.h"
#include "date_utils.h"
#include "unicode.h"
#include "lv5/error_check.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
namespace iv {
namespace lv5 {

inline JSVal Print(const Arguments& args, Error* e) {
  if (args.size() > 0) {
    Context* const ctx = args.ctx();
    for (Arguments::const_iterator it = args.begin(),
         last = args.end(); it != last;) {
      const JSString* const str = it->ToString(ctx, IV_LV5_ERROR(e));
      core::unicode::FPutsUTF16(stdout, str->begin(), str->end());
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

inline JSVal Quit(const Arguments& args, Error* e) {
  int code = 0;
  if (args.size() > 0) {
    const double val = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    code = core::DoubleToInt32(val);
  }
  std::exit(code);
  return JSUndefined;
}

inline JSVal HiResTime(const Arguments& args, Error* e) {
  return date::HighResTime();
}

inline JSVal CollectGarbage(const Arguments& args, Error* e) {
  GC_gcollect();
  return JSUndefined;
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_COMMAND_H_
