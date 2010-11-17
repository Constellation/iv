#ifndef _IV_LV5_PRINT_H_
#define _IV_LV5_PRINT_H_
#include <iostream>  // NOLINT
#include "jsval.h"
#include "error.h"
#include "arguments.h"
#include "icu/ustream.h"
namespace iv {
namespace lv5 {

inline JSVal Print(const Arguments& args, Error* error) {
  if (args.args().size() > 0) {
    const std::size_t last_index = args.size() - 1;
    std::size_t index = 0;
    for (Arguments::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it, ++index) {
      const JSVal& val = *it;
      const JSString* const str = val.ToString(args.ctx(), error);
      if (*error) {
        return JSUndefined;
      }
      std::cout << str->data() << ((index == last_index) ? "\n" : " ");
    }
    std::cout << std::flush;
  }
  return JSUndefined;
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_PRINT_H_
