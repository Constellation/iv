#ifndef _IV_LV5_RUNTIME_RAILGUN_H_
#define _IV_LV5_RUNTIME_RAILGUN_H_
#include "lv5/error_check.h"
#include "lv5/internal.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/utility.h"
#include "lv5/railgun/jsfunction.h"
namespace iv {
namespace lv5 {
namespace railgun {

inline JSVal FunctionConstructor(const Arguments& args, Error* e) {
  Context* const ctx = static_cast<Context*>(args.ctx());
  StringBuilder builder;
  internal::BuildFunctionSource(&builder, args, IV_LV5_ERROR(e));
  JSString* const source = builder.Build(ctx);
  Code* const code = CompileFunction(ctx, source, false, true, IV_LV5_ERROR(e));
  return JSVMFunction::New(ctx, code, ctx->global_env());
}

inline JSVal GlobalEval(const Arguments& args, Error* e) {
  return JSUndefined;
}

inline JSVal DirectCallToEval(const Arguments& args, Error* e) {
  return JSUndefined;
}

} } }  // iv::lv5::railgun
#endif  // _IV_LV5_RUNTIME_RAILGUN_H_
