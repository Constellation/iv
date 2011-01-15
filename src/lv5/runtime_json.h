#ifndef _IV_LV5_RUNTIME_JSON_H_
#define _IV_LV5_RUNTIME_JSON_H_
#include <vector>
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

inline JSVal Walk(Context* ctx, JSObject* holder,
                  Symbol name, JSFunction* reviver, Error* e) {
  const JSVal val = holder->Get(ctx, name, ERROR(e));
  if (val.IsObject()) {
    JSObject* const obj = val.object();
    if (ctx->Cls("Array").name == obj->class_name()) {
      JSArray* const ary = static_cast<JSArray*>(obj);
      const JSVal length = ary->Get(ctx, ctx->length_symbol(), ERROR(e));
      const double temp = length.ToNumber(ctx, ERROR(e));
      const uint32_t len = core::DoubleToUInt32(temp);
      for (uint32_t i = 0; i < len; ++i) {
        const JSVal new_element = Walk(ctx, ary, ctx->InternIndex(i), reviver, ERROR(e));
        if (new_element.IsUndefined()) {
          ary->DeleteWithIndex(ctx, i, false, ERROR(e));
        } else {
          ary->DefineOwnPropertyWithIndex(
              ctx, i,
              DataDescriptor(new_element,
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
              false, ERROR(e));
        }
      }
    } else {
      std::vector<Symbol> keys;
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kExcludeNotEnumerable);
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it) {
        const JSVal new_element = Walk(ctx, obj, *it, reviver, ERROR(e));
        if (new_element.IsUndefined()) {
          obj->Delete(ctx, *it, false, ERROR(e));
        } else {
          obj->DefineOwnProperty(
              ctx, *it,
              DataDescriptor(new_element,
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
              false, ERROR(e));
        }
      }
    }
  }
  Arguments args_list(ctx, holder, 2);
  args_list[0] = ctx->ToString(name);
  args_list[1] = val;
  return reviver->Call(args_list, e);
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
  const JSVal result = detail::ParseJSON(ctx, *text, ERROR(e));
  if (args_size > 1 && args[1].IsCallable()) {
    JSObject* const root = JSObject::New(ctx);
    const Symbol empty = ctx->Intern("");
    root->DefineOwnProperty(
        ctx, empty,
        DataDescriptor(result,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, ERROR(e));
    return detail::Walk(ctx, root, empty, args[1].object()->AsCallable(), e);
  }
  return result;
}
} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_JSON_H_
