#ifndef _IV_LV5_COMMAND_H_
#define _IV_LV5_COMMAND_H_
#include <iostream>  // NOLINT
#include <cstdlib>
#include "conversions.h"
#include "lv5/error_check.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
namespace iv {
namespace lv5 {

inline JSVal Print(const Arguments& args, Error* e) {
  if (args.size() > 0) {
    Context* const ctx = args.ctx();
    const std::size_t last_index = args.size() - 1;
    std::size_t index = 0;
    for (Arguments::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it, ++index) {
      const JSString* const str = it->ToString(ctx, IV_LV5_ERROR(e));
      std::cout << *str << ((index == last_index) ? "\n" : " ");
    }
    std::cout << std::flush;
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

} }  // namespace iv::lv5
#endif  // _IV_LV5_COMMAND_H_
