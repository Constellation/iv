#ifndef IV_LV5_RUNTIME_H_
#define IV_LV5_RUNTIME_H_
#include "arguments.h"
#include "jsval.h"

namespace iv {
namespace lv5 {

class Error;

JSVal Runtime_ObjectConstructor(const Arguments& args, Error* error);
JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, Error* error);
JSVal Runtime_ObjectToString(const Arguments& args, Error* error);
JSVal Runtime_FunctionToString(const Arguments& args, Error* error);
JSVal Runtime_ErrorToString(const Arguments& args, Error* error);

} }  // namespace iv::lv5
#endif  // IV_LV5_RUNTIME_H_
