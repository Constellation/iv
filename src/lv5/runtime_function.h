#ifndef _IV_LV5_RUNTIME_FUNCTION_H_
#define _IV_LV5_RUNTIME_FUNCTION_H_
#include <algorithm>
#include "ustring.h"
#include "ustringpiece.h"
#include "jsfunction.h"
#include "arguments.h"
#include "jsval.h"
#include "jsstring.h"
#include "error.h"
#include "lv5.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

static const std::string kFunctionPrefix("function ");

inline void CheckFunctionExpressionIsOne(const FunctionLiteral& func, Error* e) {
  const FunctionLiteral::Statements& stmts = func.body();
  if (stmts.size() == 1) {
    const Statement& stmt = *stmts[0];
    if (stmt.AsExpressionStatement()) {
      if (stmt.AsExpressionStatement()->expr()->AsFunctionLiteral()) {
        return;
      }
    }
  }
  e->Report(Error::Syntax,
            "Function Constructor with invalid arguments");
}

}  // namespace iv::lv5::runtime::detail

inline JSVal FunctionPrototype(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype", args, error);
  return JSUndefined;
}

inline JSVal FunctionConstructor(const Arguments& args, Error* error) {
  const std::size_t arg_count = args.size();
  Context* const ctx = args.ctx();
  JSStringBuilder builder(ctx);
  if (arg_count == 0) {
    builder.Append("(function() { \n})");
  } else if (arg_count == 1) {
    builder.Append("(function() { ");
    JSString* const str = args[0].ToString(ctx, ERROR(error));
    builder.Append(*str);
    builder.Append("\n})");
  } else {
    builder.Append("(function(");
    Arguments::const_iterator it = args.begin();
    const Arguments::const_iterator last = args.end() - 1;
    do {
      JSString* const str = it->ToString(ctx, ERROR(error));
      builder.Append(*str);
      ++it;
      if (it != last) {
        builder.Append(',');
      } else {
        break;
      }
    } while (true);
    builder.Append(") { ");
    JSString* const prog = last->ToString(ctx, ERROR(error));
    builder.Append(*prog);
    builder.Append("\n})");
  }
  JSString* const source = builder.Build();
  JSScript* const script = CompileScript(args.ctx(), source,
                                         false, ERROR(error));
  detail::CheckFunctionExpressionIsOne(*script->function(), ERROR(error));
  const Interpreter::ContextSwitcher switcher(ctx,
                                              ctx->global_env(),
                                              ctx->global_env(),
                                              ctx->global_obj(),
                                              false);
  ctx->Run(script);
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

inline JSVal FunctionToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction() || func->AsBoundFunction()) {
      return JSString::NewAsciiString(args.ctx(),
                                      "function native() { [native code] }");
    } else {
      JSStringBuilder builder(args.ctx());
      builder.Append(detail::kFunctionPrefix);
      if (const core::Maybe<const Identifier> name = func->AsCodeFunction()->name()) {
        builder.Append((*name).value());
      } else {
        builder.Append("anonymous");
      }
      builder.Append(func->AsCodeFunction()->GetSource());
      return builder.Build();
    }
  }
  error->Report(Error::Type,
                "Function.prototype.toString is not generic function");
  return JSUndefined;
}

// section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
inline JSVal FunctionApply(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype.apply", args, error);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    Context* const ctx = args.ctx();
    const std::size_t args_size = args.size();
    if (args_size < 2) {
      if (args_size == 0) {
        return func->Call(Arguments(ctx, JSUndefined), error);
      } else {
        return func->Call(Arguments(ctx, args[0]), error);
      }
    }
    const JSVal& second = args[1];
    if (!second.IsObject()) {
      error->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSUndefined;
    }
    JSObject* const arg_array = second.object();
    const JSVal len = arg_array->Get(ctx, ctx->length_symbol(), ERROR(error));
    if (len.IsUndefined() || len.IsNull()) {
      error->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSUndefined;
    }
    const double temp = len.ToNumber(ctx, ERROR(error));
    const uint32_t n = core::DoubleToUInt32(temp);
    if (n != temp) {
      error->Report(
          Error::Type,
          "Function.prototype.apply requires Arraylike as 2nd arguments");
      return JSUndefined;
    }

    Arguments args_list(ctx, n);
    uint32_t index = 0;
    while (index < n) {
        args_list[index] = arg_array->GetWithIndex(
            ctx,
            index, ERROR(error));
        ++index;
    }
    args_list.set_this_binding(args[0]);
    return func->Call(args_list, error);
  }
  error->Report(Error::Type,
                "Function.prototype.apply is not generic function");
  return JSUndefined;
}

// section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
inline JSVal FunctionCall(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype.call", args, error);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    using std::copy;
    JSFunction* const func = obj.object()->AsCallable();
    Context* const ctx = args.ctx();
    const std::size_t args_size = args.size();

    Arguments args_list(ctx, (args_size > 1) ? args_size - 1 : 0);

    if (args_size > 1) {
      copy(args.begin() + 1, args.end(), args_list.begin());
    }

    if (args_size > 0) {
      args_list.set_this_binding(args[0]);
    } else {
      args_list.set_this_binding(JSUndefined);
    }
    return func->Call(args_list, error);
  }
  error->Report(Error::Type,
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
