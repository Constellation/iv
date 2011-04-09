#ifndef _IV_LV5_RUNTIME_FUNCTION_H_
#define _IV_LV5_RUNTIME_FUNCTION_H_
#include <algorithm>
#include <tr1/type_traits>
#include "enable_if.h"
#include "ustring.h"
#include "ustringpiece.h"
#include "lv5/lv5.h"
#include "lv5/jsfunction.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/context_utils.h"
#include "lv5/teleporter_jsfunction.h"
#include "lv5/error.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

static const std::string kFunctionPrefix("function ");

struct NativeFunction {
  enum SpecificationMode {
    kSpecificationStrict,
    kSpecificationCompatibility
  };

  template<SpecificationMode mode>
  static JSVal ToString(Context* ctx, JSFunction* func, Error* e,
      typename enable_if_c<mode == kSpecificationStrict>::type* = 0) {
    return JSString::NewAsciiString(ctx,
                                    "function native() { [native code] }");
//    const JSVal name = func->Get(ctx, context::Intern(ctx, "name"), ERROR(e));
//    assert(name.IsString());
//    StringBuilder builder;
//    builder.Append(detail::kFunctionPrefix);
//    builder.Append(*name.string());
//    builder.Append("() { /* native code */ }");
//    return builder.Build(ctx);
  }

  template<SpecificationMode mode>
  static JSVal ToString(Context* ctx, JSFunction* func, Error* e,
      typename enable_if_c<mode == kSpecificationCompatibility>::type* = 0) {
    return JSString::NewAsciiString(ctx,
                                    "function native() { [native code] }");
  }
};

}  // namespace iv::lv5::runtime::detail

inline JSVal FunctionPrototype(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Function.prototype", args, e);
  return JSUndefined;
}


// section 15.3.4.2 Function.prototype.toString()
inline JSVal FunctionToString(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Function.prototype.toString", args, e);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->IsNativeFunction()) {
      return
          detail::NativeFunction::ToString<
            detail::NativeFunction::kSpecificationStrict>(args.ctx(), func, e);
    } else {
      StringBuilder builder;
      builder.Append(detail::kFunctionPrefix);
      const core::UStringPiece name = func->GetName();
      if (name.empty()) {
        builder.Append("anonymous");
      } else {
        builder.Append(name);
      }
      builder.Append(func->GetSource());
      return builder.Build(args.ctx());
    }
  }
  e->Report(Error::Type,
            "Function.prototype.toString is not generic function");
  return JSUndefined;
}

// section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
inline JSVal FunctionApply(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Function.prototype.apply", args, e);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    Context* const ctx = args.ctx();
    const std::size_t args_size = args.size();
    if (args_size < 2) {
      ScopedArguments a(ctx, 0, ERROR(e));
      if (args_size == 0) {
        return func->Call(&a, JSUndefined, e);
      } else {
        return func->Call(&a, args[0], e);
      }
    }
    const JSVal& second = args[1];
    if (!second.IsObject()) {
      e->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSUndefined;
    }
    JSObject* const arg_array = second.object();
    const JSVal len = arg_array->Get(ctx, context::length_symbol(ctx), ERROR(e));
    if (len.IsUndefined() || len.IsNull()) {
      e->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSUndefined;
    }
    const double temp = len.ToNumber(ctx, ERROR(e));
    const uint32_t n = core::DoubleToUInt32(temp);
    if (n != temp) {
      e->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSUndefined;
    }

    ScopedArguments args_list(ctx, n, ERROR(e));
    uint32_t index = 0;
    while (index < n) {
        args_list[index] = arg_array->GetWithIndex(
            ctx,
            index, ERROR(e));
        ++index;
    }
    return func->Call(&args_list, args[0], e);
  }
  e->Report(Error::Type,
            "Function.prototype.apply is not generic function");
  return JSUndefined;
}

// section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
inline JSVal FunctionCall(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Function.prototype.call", args, e);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    Context* const ctx = args.ctx();
    const std::size_t args_size = args.size();

    ScopedArguments args_list(ctx, (args_size > 1) ? args_size - 1 : 0, ERROR(e));

    if (args_size > 1) {
      std::copy(args.begin() + 1, args.end(), args_list.begin());
    }

    const JSVal this_binding = (args_size > 0) ? args[0] : JSUndefined;
    return func->Call(&args_list, this_binding, e);
  }
  e->Report(Error::Type,
            "Function.prototype.call is not generic function");
  return JSUndefined;
}

// section 15.3.4.5 Function.prototype.bind(thisArg[, arg1[, arg2, ...]])
inline JSVal FunctionBind(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype.bind", args, error);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    const std::size_t args_size = args.size();
    JSFunction* const target = obj.object()->AsCallable();
    JSVal this_binding;
    if (args_size == 0) {
      this_binding = JSUndefined;
    } else {
      this_binding = args[0];
    }
    return JSBoundFunction::New(args.ctx(), target, this_binding, args);
  }
  error->Report(Error::Type,
                "Function.prototype.bind is not generic function");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_FUNCTION_H_
