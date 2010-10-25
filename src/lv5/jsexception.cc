#include "jsexception.h"
#include "jsval.h"

namespace iv {
namespace lv5 {

JSError::JSError(Error::Code code)
  : code_(code) {
}

JSVal JSError::CreateFromCode(Error::Code code) {
  return new JSError(code);
}

} }  // namespace iv::lv5
