#ifndef IV_LV5_RUNTIME_H_
#define IV_LV5_RUNTIME_H_
#include "arguments.h"
#include "jsval.h"

namespace iv {
namespace lv5 {

class Error;

JSVal Runtime_ThrowTypeError(const Arguments& args, Error* error);

JSVal Runtime_ObjectConstructor(const Arguments& args, Error* error);
JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, Error* error);
JSVal Runtime_ObjectToString(const Arguments& args, Error* error);

JSVal Runtime_FunctionToString(const Arguments& args, Error* error);

JSVal Runtime_ErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_NativeErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_EvalErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_RangeErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_ReferenceErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_SyntaxErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_TypeErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_URIErrorConstructor(const Arguments& args, Error* error);
JSVal Runtime_ErrorToString(const Arguments& args, Error* error);

JSVal Runtime_NumberConstructor(const Arguments& args, Error* error);

JSVal Runtime_MathAbs(const Arguments& args, Error* error);
JSVal Runtime_MathAcos(const Arguments& args, Error* error);
JSVal Runtime_MathAsin(const Arguments& args, Error* error);
JSVal Runtime_MathAtan(const Arguments& args, Error* error);
JSVal Runtime_MathAtan2(const Arguments& args, Error* error);
JSVal Runtime_MathCeil(const Arguments& args, Error* error);
JSVal Runtime_MathCos(const Arguments& args, Error* error);
JSVal Runtime_MathExp(const Arguments& args, Error* error);
JSVal Runtime_MathFloor(const Arguments& args, Error* error);
JSVal Runtime_MathLog(const Arguments& args, Error* error);
JSVal Runtime_MathMax(const Arguments& args, Error* error);
JSVal Runtime_MathMin(const Arguments& args, Error* error);
JSVal Runtime_MathPow(const Arguments& args, Error* error);
JSVal Runtime_MathRandom(const Arguments& args, Error* error);
JSVal Runtime_MathRound(const Arguments& args, Error* error);
JSVal Runtime_MathSin(const Arguments& args, Error* error);
JSVal Runtime_MathSqrt(const Arguments& args, Error* error);
JSVal Runtime_MathTan(const Arguments& args, Error* error);

} }  // namespace iv::lv5
#endif  // IV_LV5_RUNTIME_H_
