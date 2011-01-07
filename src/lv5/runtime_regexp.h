#ifndef _IV_LV5_RUNTIME_REGEXP_H_
#define _IV_LV5_RUNTIME_REGEXP_H_
#include "arguments.h"
#include "jsval.h"
#include "context.h"
#include "error.h"
#include "lv5.h"
#include "jsregexp.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

}  // namespace iv::lv5::runtime::detail

inline JSVal RegExpConstructor(const Arguments& args, Error* error) {
  return JSRegExp::New(args.ctx());
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_REGEXP_H_
