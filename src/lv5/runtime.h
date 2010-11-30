#ifndef IV_LV5_RUNTIME_H_
#define IV_LV5_RUNTIME_H_
#include "arguments.h"
#include "jsval.h"

namespace iv {
namespace lv5 {

class Error;

JSVal Runtime_GlobalEval(const Arguments& args, Error* error);
JSVal Runtime_DirectCallToEval(const Arguments& args, Error* error);
JSVal Runtime_InDirectCallToEval(const Arguments& args, Error* error);
JSVal Runtime_GlobalParseInt(const Arguments& args, Error* error);
JSVal Runtime_GlobalParseFloat(const Arguments& args, Error* error);
JSVal Runtime_GlobalIsNaN(const Arguments& args, Error* error);
JSVal Runtime_GlobalIsFinite(const Arguments& args, Error* error);

JSVal Runtime_ThrowTypeError(const Arguments& args, Error* error);

JSVal Runtime_ObjectConstructor(const Arguments& args, Error* error);
JSVal Runtime_ObjectGetPrototypeOf(const Arguments& args, Error* error);
JSVal Runtime_ObjectGetOwnPropertyDescriptor(const Arguments& args,
                                             Error* error);
JSVal Runtime_ObjectGetOwnPropertyNames(const Arguments& args, Error* error);
JSVal Runtime_ObjectCreate(const Arguments& args, Error* error);
JSVal Runtime_ObjectDefineProperty(const Arguments& args, Error* error);
JSVal Runtime_ObjectDefineProperties(const Arguments& args, Error* error);
JSVal Runtime_ObjectSeal(const Arguments& args, Error* error);
JSVal Runtime_ObjectFreeze(const Arguments& args, Error* error);
JSVal Runtime_ObjectPreventExtensions(const Arguments& args, Error* error);
JSVal Runtime_ObjectIsSealed(const Arguments& args, Error* error);
JSVal Runtime_ObjectIsFrozen(const Arguments& args, Error* error);
JSVal Runtime_ObjectIsExtensible(const Arguments& args, Error* error);
JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, Error* error);
JSVal Runtime_ObjectToString(const Arguments& args, Error* error);

JSVal Runtime_FunctionPrototype(const Arguments& args, Error* error);
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

JSVal Runtime_StringConstructor(const Arguments& args, Error* error);
JSVal Runtime_StringFromCharCode(const Arguments& args, Error* error);
JSVal Runtime_StringToString(const Arguments& args, Error* error);
JSVal Runtime_StringValueOf(const Arguments& args, Error* error);
JSVal Runtime_StringCharAt(const Arguments& args, Error* error);
JSVal Runtime_StringCharCodeAt(const Arguments& args, Error* error);
JSVal Runtime_StringConcat(const Arguments& args, Error* error);
JSVal Runtime_StringIndexOf(const Arguments& args, Error* error);
JSVal Runtime_StringLastIndexOf(const Arguments& args, Error* error);

JSVal Runtime_BooleanConstructor(const Arguments& args, Error* error);
JSVal Runtime_BooleanToString(const Arguments& args, Error* error);
JSVal Runtime_BooleanValueOf(const Arguments& args, Error* error);

JSVal Runtime_NumberConstructor(const Arguments& args, Error* error);
JSVal Runtime_NumberToString(const Arguments& args, Error* error);
JSVal Runtime_NumberValueOf(const Arguments& args, Error* error);

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
