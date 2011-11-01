#ifndef IV_LV5_RAILGUN_FWD_H_
#define IV_LV5_RAILGUN_FWD_H_
#include <iv/lv5/jsval.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Operation;
class VM;
class CodeContext;
class Code;
struct Frame;
class Stack;

class VariableScope;
class FunctionScope;
class WithScope;
class CatchScope;

class Compiler;
class Context;

template<typename Derived>
class DisAssembler;

JSVal FunctionConstructor(const Arguments& args, Error* e);
JSVal GlobalEval(const Arguments& args, Error* e);
inline JSVal DirectCallToEval(const Arguments& args, Frame* frame, Error* e);

class JSScript;

template<typename Source>
class JSEvalScript;

class JSGlobalScript;

class JSVMFunction;


Code* CompileFunction(Context* ctx, const JSString* str, Error* e);

void Instantiate(Context* ctx,
                 Code* code,
                 Frame* frame,
                 bool is_eval,
                 bool is_global_env,
                 JSVMFunction* func,  // maybe NULL
                 Error* e);

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_FWD_H_
