#ifndef _IV_LV5_RUNTIME_FUNCTION_H_
#define _IV_LV5_RUNTIME_FUNCTION_H_
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

}  // namespace iv::lv5::runtime::detail

inline JSVal FunctionPrototype(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype", args, error);
  return JSUndefined;
}

// TODO(Constellation) implement it
inline JSVal FunctionConstructor(const Arguments& args, Error* error) {
  return JSUndefined;
}

inline JSVal FunctionToString(const Arguments& args,
                              Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction()) {
      return JSString::NewAsciiString(args.ctx(),
                                      "function () { [native code] }");
    } else {
      core::UString buffer(detail::kFunctionPrefix.begin(),
                           detail::kFunctionPrefix.end());
      if (func->AsCodeFunction()->name()) {
        const core::UStringPiece name = func->AsCodeFunction()->name()->value();
        buffer.append(name.data(), name.size());
      }
      const core::UStringPiece src = func->AsCodeFunction()->GetSource();
      buffer.append(src.data(), src.size());
      return JSString::New(args.ctx(), buffer);
    }
  }
  error->Report(Error::Type,
                "Function.prototype.toString is not generic function");
  return JSUndefined;
}

inline JSVal FunctionCall(const Arguments& args,
                          Error* error) {
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

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_FUNCTION_H_
