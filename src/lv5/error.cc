#include "error.h"

namespace iv {
namespace lv5 {

void Error::Report(Code code, const core::StringPiece& str) {
  code_ = code;
  str.CopyToString(&detail_);
}

void Error::Report(const JSVal& val) {
  code_ = User;
  value_ = val;
}

void Error::Clear() {
  code_ = Normal;
}

JSVal Error::Detail() const {
  if (code_ == User) {
    return value_;
  } else {
    return JSUndefined;
  }
}

} }  // namespace iv::lv5
