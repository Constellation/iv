#ifndef IV_LV5_RUNTIME_BOOLEAN_H_
#define IV_LV5_RUNTIME_BOOLEAN_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.6.1.1 Boolean(value)
// section 15.6.2.1 new Boolean(value)
JSVal BooleanConstructor(const Arguments& args, Error* e);

// section 15.6.4.2 Boolean.prototype.toString()
JSVal BooleanToString(const Arguments& args, Error* e);

// section 15.6.4.3 Boolean.prototype.valueOf()
JSVal BooleanValueOf(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_BOOLEAN_H_
