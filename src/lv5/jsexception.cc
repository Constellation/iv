#include "jsexception.h"
#include "jsval.h"

namespace iv {
namespace lv5 {

JSError::JSError(JSErrorCode::Type code)
  : code_(code) {
}

JSVal JSError::CreateFromCode(JSErrorCode::Type code) {
  return new JSError(code);
}

} }  // namespace iv::lv5
