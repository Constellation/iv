#ifndef IV_LV5_RUNTIME_H_
#define IV_LV5_RUNTIME_H_
#include "jserrorcode.h"
#include "arguments.h"
#include "jsval.h"

namespace iv {
namespace lv5 {

JSVal Runtime_ObjectConstructor(const Arguments& args, JSErrorCode::Type* error);
JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, JSErrorCode::Type* error);
JSVal Runtime_ObjectToString(const Arguments& args, JSErrorCode::Type* error);
JSVal Runtime_FunctionToString(const Arguments& args, JSErrorCode::Type* error);

} }  // namespace iv::lv5
#endif  // IV_LV5_RUNTIME_H_
