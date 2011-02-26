#ifndef _IV_LV5_RUNTIME_JSON_H_
#define _IV_LV5_RUNTIME_JSON_H_
#include <vector>
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/jsarray.h"
#include "lv5/jsstring.h"
#include "lv5/internal.h"
#include "lv5/gc_template.h"
#include "lv5/json_lexer.h"
#include "lv5/json_parser.h"
#include "lv5/json_stringifier.h"
#include "lv5/runtime_object.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

inline JSVal ParseJSON(Context* ctx, const JSString& str, Error* e) {
  const EvalSource src(str);
  JSONParser<EvalSource> parser(ctx, src);
  return parser.Parse(e);
}

inline JSVal JSONWalk(Context* ctx, JSObject* holder,
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
        const JSVal new_element = JSONWalk(ctx, ary, ctx->InternIndex(i),
                                           reviver, ERROR(e));
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
        const JSVal new_element = JSONWalk(ctx, obj, *it, reviver, ERROR(e));
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
  Arguments args_list(ctx, 2, ERROR(e));
  args_list[0] = ctx->ToString(name);
  args_list[1] = val;
  return reviver->Call(args_list, holder, e);
}

template<typename T>
class PropertyListEqual {
 public:
  explicit PropertyListEqual(const T* target) : target_(*target) { }
  bool operator()(const T* val) const {
    return *val == target_;
  }
 private:
  const T& target_;
};

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
    return detail::JSONWalk(ctx, root, empty,
                            args[1].object()->AsCallable(), e);
  }
  return result;
}

// section 15.12.3 stringify(value[, replacer[, space]])
inline JSVal JSONStringify(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("JSON.stringify", args, e);
  const std::size_t args_size = args.size();
  Context* const ctx = args.ctx();
  JSVal value, replacer, space;
  trace::Vector<JSString*>::type property_list;
  trace::Vector<JSString*>::type* maybe = NULL;
  JSFunction* replacer_function = NULL;
  if (args_size > 0) {
    value = args[0];
    if (args_size > 1) {
      replacer = args[1];
      if (args_size > 2) {
        space = args[2];
      }
    }
  }
  // step 4
  if (replacer.IsObject()) {
    JSObject* const rep = replacer.object();
    if (replacer.IsCallable()) {  // 4-a
      replacer_function = rep->AsCallable();
    } else if (ctx->IsArray(*rep)) {  // 4-b
      using std::find_if;
      maybe = &property_list;
      const JSVal length = rep->Get(
          ctx,
          ctx->length_symbol(), ERROR(e));
      const double val = length.ToNumber(ctx, ERROR(e));
      const uint32_t len = core::DoubleToUInt32(val);
      for (uint32_t i = 0; i < len; ++i) {
        const JSVal v = rep->GetWithIndex(ctx, i, ERROR(e));
        JSString* item = NULL;
        if (v.IsString()) {
          item = v.string();
        } else if (v.IsNumber()) {
          item = v.ToString(ctx, ERROR(e));
        } else if (v.IsObject()) {
          JSObject* target = v.object();
          if (target->class_name() == ctx->Intern("String") ||
              target->class_name() == ctx->Intern("Number")) {
            item = v.ToString(ctx, ERROR(e));
          }
        }
        if (item) {
          if (find_if(
                  property_list.begin(),
                  property_list.end(),
                  detail::PropertyListEqual<JSString>(item))
              != property_list.end()) {
            property_list.push_back(v.string());
          }
        }
      }
    }
  }

  // step 5
  if (space.IsObject()) {
    JSObject* const target = space.object();
    if (target->class_name() == ctx->Intern("Number")) {
      space = space.ToNumber(ctx, ERROR(e));
    } else if (target->class_name() == ctx->Intern("String")) {
      space = space.ToString(ctx, ERROR(e));
    }
  }

  // step 6, 7, 8
  core::UString gap;
  if (space.IsNumber()) {
    const double sp = std::min<double>(10.0,
                                       core::DoubleToInteger(space.number()));
    if (sp > 0) {
      gap.assign(core::DoubleToUInt32(sp), static_cast<uc16>(' '));
    }
  } else if (space.IsString()) {
    JSString* target = space.string();
    if (target->size() <= 10) {
      gap.assign(target->data(), target->size());
    } else {
      gap.assign(target->data(), 10);
    }
  }

  // step 9
  JSObject* const wrapper = JSObject::New(ctx);

  // step 10
  const Symbol empty = ctx->Intern("");
  wrapper->DefineOwnProperty(
      ctx, empty,
      DataDescriptor(value,
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::ENUMERABLE |
                     PropertyDescriptor::CONFIGURABLE), false, ERROR(e));
  JSONStringifier stringifier(ctx, replacer_function, gap, maybe);
  return stringifier.Stringify(empty, wrapper, e);
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_JSON_H_
