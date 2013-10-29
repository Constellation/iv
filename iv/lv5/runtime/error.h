#ifndef IV_LV5_RUNTIME_ERROR_H_
#define IV_LV5_RUNTIME_ERROR_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.11.1.1 Error(message)
// section 15.11.2.1 new Error(message)
JSVal ErrorConstructor(const Arguments& args, Error* e);

// section 15.11.4.4 Error.prototype.toString()
JSVal ErrorToString(const Arguments& args, Error* e);

// section 15.11.6.1 EvalError
JSVal EvalErrorConstructor(const Arguments& args, Error* e);

// section 15.11.6.2 RangeError
JSVal RangeErrorConstructor(const Arguments& args, Error* e);

// section 15.11.6.3 ReferenceError
JSVal ReferenceErrorConstructor(const Arguments& args, Error* e);

// section 15.11.6.4 SyntaxError
JSVal SyntaxErrorConstructor(const Arguments& args, Error* e);

// section 15.11.6.5 TypeError
JSVal TypeErrorConstructor(const Arguments& args, Error* e);

// section 15.11.6.6 URIError
JSVal URIErrorConstructor(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_ERROR_H_
