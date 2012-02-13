#ifndef IV_LV5_RAILGUN_UTILITY_H_
#define IV_LV5_RAILGUN_UTILITY_H_
#include <iostream>
#include <iv/detail/memory.h>
#include <iv/parser.h>
#include <iv/unicode.h>
#include <iv/lv5/factory.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/error.h>
#include <iv/lv5/eval_source.h>
#include <iv/lv5/railgun/context.h>
#include <iv/lv5/railgun/jsscript.h>
#include <iv/lv5/railgun/jsfunction.h>
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline void InitThisBinding(Context* ctx, Frame* frame, Error* e) {
  const JSVal this_value = frame->GetThis();
  if (!frame->code()->strict()) {
    if (this_value.IsNullOrUndefined()) {
      frame->set_this_binding(ctx->global_obj());
      return;
    } else if (!this_value.IsObject()) {
      JSObject* const obj = this_value.ToObject(ctx, IV_LV5_ERROR_VOID(e));
      frame->set_this_binding(obj);
      return;
    }
  }
  frame->set_this_binding(this_value);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_UTILITY_H_
