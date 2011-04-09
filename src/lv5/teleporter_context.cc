#include "lv5/jserror.h"
#include "lv5/teleporter.h"

namespace iv {
namespace lv5 {
namespace teleporter {

bool Context::Run(teleporter::JSScript* script) {
  const ScriptScope scope(this, script);
  interp_.Run(script->function(),
              script->type() == teleporter::JSScript::kEval);
  assert(!ret_.IsEmpty() || error_);
  return error_;
}

JSVal Context::ErrorVal() {
  return JSError::Detail(this, &error_);
}

} } }  // namespace iv::lv5::teleporter
