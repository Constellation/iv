#ifndef _IV_LV5_RAILGUN_JSFUNCTION_H_
#define _IV_LV5_RAILGUN_JSFUNCTION_H_
#include "ustringpiece.h"
#include "lv5/railgun_fwd.h"
#include "lv5/arguments.h"
#include "lv5/jsenv.h"
#include "lv5/jsscript.h"
#include "lv5/jsfunction.h"
namespace iv {
namespace lv5 {
namespace railgun {

class JSVMFunction : public JSFunction {
 public:
  JSVMFunction(Context* ctx,
               const railgun::Code* code,
               JSScript* script,
               JSEnv* env);

  JSVal Call(Arguments* args,
             const JSVal& this_binding,
             Error* error);

  JSVal Construct(Arguments* args, Error* error);

  JSEnv* scope() const {
    return env_;
  }

  static JSVMFunction* New(Context* ctx,
                           const railgun::Code* code,
                           JSScript* script,
                           JSEnv* env) {
    return new JSVMFunction(ctx, code, script, env);
  }

  bool IsNativeFunction() const {
    return false;
  }

  core::UStringPiece GetSource() const;

  bool IsStrict() const {
    return false;
  }

 private:
  const railgun::Code* code_;
  JSScript* script_;
  JSEnv* env_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_JSFUNCTION_H_
