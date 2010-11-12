#include "jsregexp-impl.h"
#include "jsregexp.h"
#include "jsast.h"
namespace iv {
namespace lv5 {

JSRegExp::JSRegExp(const core::UStringPiece& value,
                   const core::UStringPiece& flags)
  : status_(U_ZERO_ERROR),
    impl_(new JSRegExpImpl(value, flags, &status_)) {
}

JSRegExp::JSRegExp(const JSRegExpImpl& reg)
  : status_(U_ZERO_ERROR),
    impl_(&reg) {
}

JSRegExp* JSRegExp::New(const core::UStringPiece& value,
                        const core::UStringPiece& flags) {
  return new JSRegExp(value, flags);
}

JSRegExp* JSRegExp::New(const RegExpLiteral* reg) {
  return new JSRegExp(reg->regexp());
}

} }  // namespace iv::lv5
