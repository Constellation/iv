#include <boost/foreach.hpp>
#include "regexp-icu.h"
#include "jsregexp.h"
namespace iv {
namespace lv5 {

JSRegExpImpl::JSRegExpImpl(const core::UStringPiece& value,
                           const core::UStringPiece& flags,
                           UErrorCode* status)
  : regexp_(NULL),
    global_(false) {
  uint32_t flagbits = 0;
  BOOST_FOREACH(const uc16& c, flags) {
    if (c == 'g') {
      if (global_) {
        *status = U_REGEX_RULE_SYNTAX;
        break;
      } else {
        global_ = true;
      }
    } else if (c == 'm') {
      if (flagbits & UREGEX_MULTILINE) {
        *status = U_REGEX_RULE_SYNTAX;
        break;
      } else {
        flagbits |= UREGEX_MULTILINE;
      }
    } else if (c == 'i') {
      if (flagbits & UREGEX_CASE_INSENSITIVE) {
        *status = U_REGEX_RULE_SYNTAX;
        break;
      } else {
        flagbits |= UREGEX_CASE_INSENSITIVE;
      }
    } else {
      *status = U_REGEX_RULE_SYNTAX;
      break;
    }
  }
  if (*status == U_ZERO_ERROR) {
    UParseError pstatus;
    regexp_ = uregex_open(value.data(),
                          value.size(), flagbits, &pstatus, status);
  }
}

JSRegExpImpl::~JSRegExpImpl() {
  uregex_close(regexp_);
}

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

JSRegExp* JSRegExp::New(const core::RegExpLiteral* reg) {
  // TODO(Constellation) unsafe downcast
  return new JSRegExp(static_cast<const ICURegExpLiteral*>(reg)->regexp());
}

} }  // namespace iv::lv5
