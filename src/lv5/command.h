#ifndef _IV_LV5_COMMAND_H_
#define _IV_LV5_COMMAND_H_
#include <iostream>  // NOLINT
#include <cstdlib>
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
namespace iv {
namespace lv5 {

inline JSVal Print(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const std::size_t last_index = args.size() - 1;
    std::size_t index = 0;
    for (Arguments::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it, ++index) {
      const JSVal& val = *it;
      const JSString* const str = val.ToString(args.ctx(), error);
      if (*error) {
        return JSUndefined;
      }
      std::cout << *str << ((index == last_index) ? "\n" : " ");
    }
    std::cout << std::flush;
  }
  return JSUndefined;
}

inline JSVal Quit(const Arguments& args, Error* error) {
  int code = 0;
  if (args.size() > 0) {
    const double val = args[0].ToNumber(args.ctx(), ERROR(error));
    code = core::DoubleToInt32(val);
  }
  std::exit(code);
  return JSUndefined;
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_COMMAND_H_
