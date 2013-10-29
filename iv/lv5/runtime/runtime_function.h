#ifndef IV_LV5_RUNTIME_FUNCTION_H_
#define IV_LV5_RUNTIME_FUNCTION_H_
#include <algorithm>
#include <iv/detail/type_traits.h>
#include <iv/enable_if.h>
#include <iv/ustring.h>
#include <iv/ustringpiece.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

static const std::string kFunctionPrefix("function ");

}  // namespace detail

inline JSVal FunctionPrototype(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Function.prototype", args, e);
  return JSUndefined;
}

// section 15.3.4.2 Function.prototype.toString()
inline JSVal FunctionToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Function.prototype.toString", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func =
        static_cast<JSFunction*>(obj.object());
    Context* const ctx = args.ctx();
    JSStringBuilder builder;
    builder.Append(detail::kFunctionPrefix);
    Slot slot;
    if (func->GetOwnPropertySlot(ctx, symbol::name(), &slot)) {
      const JSVal name = slot.Get(ctx, func, IV_LV5_ERROR(e));
      JSString* name_str = name.ToString(ctx, IV_LV5_ERROR(e));
      if (name_str->empty()) {
        builder.Append("anonymous");
      } else {
        builder.AppendJSString(*name_str);
      }
    } else {
      builder.Append("anonymous");
    }
    builder.Append(func->GetSource());
    return builder.Build(ctx, false, e);
  }
  e->Report(Error::Type, "Function.prototype.toString is not generic function");
  return JSEmpty;
}

// section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
inline JSVal FunctionApply(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Function.prototype.apply", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func =
        static_cast<JSFunction*>(obj.object());
    Context* const ctx = args.ctx();
    const std::size_t args_size = args.size();
    if (args_size < 2) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return func->Call(&a, args.At(0), e);
    }
    const JSVal second = args[1];
    if (!second.IsObject()) {
      e->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSEmpty;
    }
    JSObject* const arg_array = second.object();
    // 15.3.4.3 Errata
    const uint32_t len = internal::GetLength(ctx, arg_array, IV_LV5_ERROR(e));
    ScopedArguments args_list(ctx, len, IV_LV5_ERROR(e));
    for (uint32_t index = 0; index < len; ++index) {
      args_list[index] =
          arg_array->Get(ctx,
                         symbol::MakeSymbolFromIndex(index), IV_LV5_ERROR(e));
    }
    return func->Call(&args_list, args.front(), e);
  }
  e->Report(Error::Type, "Function.prototype.apply is not generic function");
  return JSEmpty;
}

// section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
inline JSVal FunctionCall(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Function.prototype.call", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func =
        static_cast<JSFunction*>(obj.object());
    const std::size_t args_size = args.size();
    ScopedArguments args_list(args.ctx(),
                              (args_size > 1) ? args_size - 1 : 0,
                              IV_LV5_ERROR(e));
    if (args_size > 1) {
      std::copy(args.begin() + 1, args.end(), args_list.begin());
    }
    return func->Call(&args_list, args.At(0), e);
  }
  e->Report(Error::Type, "Function.prototype.call is not generic function");
  return JSEmpty;
}

// section 15.3.4.5 Function.prototype.bind(thisArg[, arg1[, arg2, ...]])
inline JSVal FunctionBind(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Function.prototype.bind", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func =
        static_cast<JSFunction*>(obj.object());
    return JSBoundFunction::New(args.ctx(), func, args.At(0), args);
  }
  e->Report(Error::Type, "Function.prototype.bind is not generic function");
  return JSEmpty;
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_FUNCTION_H_
