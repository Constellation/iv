#ifndef _IV_LV5_RUNTIME_JSON_H_
#define _IV_LV5_RUNTIME_JSON_H_
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsarray.h"
#include "jsstring.h"
#include "conversions.h"
#include "internal.h"
#include "json_lexer.h"
#include "json_parser.h"
#include "runtime_object.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

inline JSVal ParseJSON(Context* ctx, const JSString& str, Error* e) {
  const EvalSource src(str);
  JSONParser<EvalSource> parser(ctx, src);
  return parser.Parse(e);
}

}  // namespace iv::lv5::runtime::detail

// section 15.12.2 parse(text[, reviver])
inline JSVal JSONParse(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("JSON.parse", args, e);
  const std::size_t args_size = args.size();
  Context* const ctx = args.ctx();
  JSVal first;
  if (args_size > 0) {
    first = args[0];
  }
  const JSString* const text = first.ToString(ctx, ERROR(e));
  return detail::ParseJSON(ctx, *text, e);
}
} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_JSON_H_
