#ifndef _IV_LV5_TELEPORTER_JSFUNCTION_IMPL_H_
#define _IV_LV5_TELEPORTER_JSFUNCTION_IMPL_H_
#include "lv5/teleporter/jsfunction.h"
#include "lv5/teleporter/interpreter.h"
namespace iv {
namespace lv5 {
namespace teleporter {

JSVal JSCodeFunction::Call(Arguments* args,
                           const JSVal& this_binding, Error* e) {
  Context* const ctx = static_cast<Context*>(args->ctx());
  args->set_this_binding(this_binding);
  ctx->interp()->Invoke(this, *args, e);
  if (ctx->mode() == Context::RETURN) {
    ctx->set_mode(Context::NORMAL);
  }
  assert(!ctx->ret().IsEmpty() || *e);
  return ctx->ret();
}

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_JSFUNCTION_IMPL_H_
