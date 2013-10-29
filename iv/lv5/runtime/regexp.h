#ifndef IV_LV5_RUNTIME_REGEXP_H_
#define IV_LV5_RUNTIME_REGEXP_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal RegExpConstructor(const Arguments& args, Error* e);

// section 15.10.6.2 RegExp.prototype.exec(string)
JSVal RegExpExec(const Arguments& args, Error* e);

// section 15.10.6.3 RegExp.prototype.test(string)
JSVal RegExpTest(const Arguments& args, Error* e);

// section 15.10.6.4 RegExp.prototype.toString()
JSVal RegExpToString(const Arguments& args, Error* e);

// Not Standard RegExp.prototype.compile(pattern, flags)
// this method is deprecated.
JSVal RegExpCompile(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_REGEXP_H_
