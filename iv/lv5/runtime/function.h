#ifndef IV_LV5_RUNTIME_FUNCTION_H_
#define IV_LV5_RUNTIME_FUNCTION_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal FunctionPrototype(const Arguments& args, Error* e);

// section 15.3.4.2 Function.prototype.toString()
JSVal FunctionToString(const Arguments& args, Error* e);

// section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
JSVal FunctionApply(const Arguments& args, Error* e);

// section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
JSVal FunctionCall(const Arguments& args, Error* e);

// section 15.3.4.5 Function.prototype.bind(thisArg[, arg1[, arg2, ...]])
JSVal FunctionBind(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_FUNCTION_H_
