#ifndef _IV_LV5_RAILGUN_FWD_H_
#define _IV_LV5_RAILGUN_FWD_H_
#include "lv5/jsval.h"
#include "lv5/arguments.h"
#include "lv5/error.h"
namespace iv {
namespace lv5 {
namespace railgun {

class VM;
class CodeContext;
class Code;

class Analyzer;

class Compiler;
class Context;

template<typename Derived>
class DisAssembler;

JSVal FunctionConstructor(const Arguments& args, Error* e);
JSVal GlobalEval(const Arguments& args, Error* e);
JSVal DirectCallToEval(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_FWD_H_
