#include <vector>
#include <iv/conversions.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/context.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/eval_source.h>
#include <iv/lv5/json.h>
#include <iv/lv5/runtime/object.h>
#include <iv/lv5/runtime/json.h>

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

JSVal JSONWalk(Context* ctx, JSObject* holder,
                      Symbol name, JSFunction* reviver, Error* e) {
  const JSVal val = holder->Get(ctx, name, IV_LV5_ERROR(e));
  if (val.IsObject()) {
    JSObject* const obj = val.object();
    if (obj->IsClass<Class::Array>()) {
      JSArray* const ary = static_cast<JSArray*>(obj);
      const uint32_t len = internal::GetLength(ctx, ary, IV_LV5_ERROR(e));
      for (uint32_t i = 0; i < len; ++i) {
        const Symbol sym = symbol::MakeSymbolFromIndex(i);
        const JSVal new_element = JSONWalk(ctx, ary, sym,
                                           reviver, IV_LV5_ERROR(e));
        if (new_element.IsUndefined()) {
          ary->JSArray::Delete(ctx, sym, false, IV_LV5_ERROR(e));
        } else {
          ary->JSArray::DefineOwnProperty(
              ctx, sym,
              DataDescriptor(new_element, ATTR::W | ATTR::E | ATTR::C),
              false, IV_LV5_ERROR(e));
        }
      }
    } else {
      PropertyNamesCollector collector;
      obj->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
      for (PropertyNamesCollector::Names::const_iterator
           it = collector.names().begin(),
           last = collector.names().end();
           it != last; ++it) {
        const Symbol sym = *it;
        const JSVal new_element =
            JSONWalk(ctx, obj, sym, reviver, IV_LV5_ERROR(e));
        if (new_element.IsUndefined()) {
          obj->Delete(ctx, sym, false, IV_LV5_ERROR(e));
        } else {
          obj->DefineOwnProperty(
              ctx, sym,
              DataDescriptor(new_element, ATTR::W | ATTR::E | ATTR::C),
              false, IV_LV5_ERROR(e));
        }
      }
    }
  }
  ScopedArguments args_list(ctx, 2, IV_LV5_ERROR(e));
  args_list[0] = JSString::New(ctx, name);
  args_list[1] = val;
  return reviver->Call(&args_list, holder, e);
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

}  // namespace detail

// section 15.12.2 parse(text[, reviver])
JSVal JSONParse(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("JSON.parse", args, e);
  Context* const ctx = args.ctx();
  JSString* const text = args.At(0).ToString(ctx, IV_LV5_ERROR(e));
  JSVal result;
  if (text->Is8Bit()) {
    result = ParseJSON<true>(ctx, *text->Flatten8(), IV_LV5_ERROR(e));
  } else {
    result = ParseJSON<true>(ctx, *text->Flatten16(), IV_LV5_ERROR(e));
  }
  const JSVal second = args.At(1);
  if (second.IsCallable()) {
    JSObject* const root = JSObject::New(ctx);
    const Symbol empty = ctx->Intern("");
    root->DefineOwnProperty(
        ctx, empty,
        DataDescriptor(result, ATTR::W | ATTR::E | ATTR::C),
        false, IV_LV5_ERROR(e));
    return detail::JSONWalk(
        ctx, root, empty, static_cast<JSFunction*>(second.object()), e);
  }
  return result;
}

// section 15.12.3 stringify(value[, replacer[, space]])
JSVal JSONStringify(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("JSON.stringify", args, e);
  Context* const ctx = args.ctx();
  trace::Vector<JSString*>::type property_list;
  trace::Vector<JSString*>::type* maybe = nullptr;
  JSFunction* replacer_function = nullptr;
  const JSVal value(args.At(0));
  const JSVal replacer(args.At(1));
  JSVal space(args.At(2));

  // step 4
  if (replacer.IsObject()) {
    JSObject* const rep = replacer.object();
    if (replacer.IsCallable()) {  // 4-a
      replacer_function = static_cast<JSFunction*>(rep);
    } else if (rep->IsClass<Class::Array>()) {  // 4-b
      maybe = &property_list;
      const uint32_t len = internal::GetLength(ctx, rep, IV_LV5_ERROR(e));
      for (uint32_t i = 0; i < len; ++i) {
        const JSVal v =
            rep->Get(ctx, symbol::MakeSymbolFromIndex(i), IV_LV5_ERROR(e));
        JSString* item = nullptr;
        if (v.IsString()) {
          item = v.string();
        } else if (v.IsNumber()) {
          item = v.ToString(ctx, IV_LV5_ERROR(e));
        } else if (v.IsObject()) {
          JSObject* target = v.object();
          if (target->IsClass<Class::String>() ||
              target->IsClass<Class::Number>()) {
            item = v.ToString(ctx, IV_LV5_ERROR(e));
          }
        }
        if (item) {
          if (std::find_if(
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
    if (target->IsClass<Class::Number>()) {
      space = space.ToNumber(ctx, IV_LV5_ERROR(e));
    } else if (target->IsClass<Class::String>()) {
      space = space.ToString(ctx, IV_LV5_ERROR(e));
    }
  }

  // step 6, 7, 8
  std::u16string gap;
  if (space.IsNumber()) {
    const double sp = std::min<double>(10.0,
                                       core::DoubleToInteger(space.number()));
    if (sp > 0) {
      gap.assign(core::DoubleToUInt32(sp), static_cast<char16_t>(' '));
    }
  } else if (space.IsString()) {
    JSString* sp = space.string();
    if (sp->Is8Bit()) {
      const JSAsciiFlatString* fiber = sp->Flatten8();
      if (fiber->size() <= 10) {
        gap.assign(fiber->begin(), fiber->end());
      } else {
        gap.assign(fiber->begin(), fiber->begin() + 10);
      }
    } else {
      const JSUTF16FlatString* fiber = sp->Flatten16();
      if (fiber->size() <= 10) {
        gap.assign(fiber->begin(), fiber->end());
      } else {
        gap.assign(fiber->begin(), fiber->begin() + 10);
      }
    }
  }

  // step 9
  JSObject* const wrapper = JSObject::New(ctx);

  // step 10
  const Symbol empty = ctx->Intern("");
  wrapper->DefineOwnProperty(
      ctx, empty,
      DataDescriptor(value, ATTR::W | ATTR::E | ATTR::C),
      false, IV_LV5_ERROR(e));
  JSONStringifier stringifier(ctx, replacer_function, gap, maybe);
  return stringifier.Stringify(empty, wrapper, e);
}

} } }  // namespace iv::lv5::runtime
