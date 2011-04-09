#ifndef _IV_LV5_TELEPORTER_JSFUNCTION_H_
#define _IV_LV5_TELEPORTER_JSFUNCTION_H_
#include "ustringpiece.h"
#include "lv5/context_utils.h"
#include "lv5/jsobject.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
#include "lv5/jsast.h"
#include "lv5/lv5.h"
#include "lv5/jsfunction.h"
#include "lv5/teleporter_fwd.h"
namespace iv {
namespace lv5 {

class JSEnv;
class JSInterpreterScript;

namespace teleporter {

class JSCodeFunction : public JSFunction {
 public:
  JSCodeFunction(Context* ctx, const FunctionLiteral* func,
                 JSInterpreterScript* script, JSEnv* env);

  JSVal Call(Arguments* args, const JSVal& this_binding, Error* e);

  JSVal Construct(Arguments* args, Error* e);

  JSEnv* scope() const {
    return env_;
  }

  const FunctionLiteral* code() const {
    return function_;
  }

  static JSCodeFunction* New(Context* ctx,
                             const FunctionLiteral* func,
                             JSInterpreterScript* script,
                             JSEnv* env) {
    return new JSCodeFunction(ctx, func, script, env);
  }

  JSCodeFunction* AsCodeFunction() {
    return this;
  }

  JSNativeFunction* AsNativeFunction() {
    return NULL;
  }

  JSBoundFunction* AsBoundFunction() {
    return NULL;
  }

  core::UStringPiece GetSource() const;

  const core::Maybe<Identifier> name() const {
    return function_->name();
  }

  bool IsStrict() const {
    return function_->strict();
  }

 private:
  const FunctionLiteral* function_;
  JSInterpreterScript* script_;
  JSEnv* env_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_JSFUNCTION_H_
