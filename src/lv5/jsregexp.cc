#include <boost/foreach.hpp>
#include "jsregexp.h"
namespace iv {
namespace lv5 {

JSRegExp::JSRegExp(const core::UStringPiece& value,
                   const core::UStringPiece& flags)
  : status_(U_ZERO_ERROR),
    impl_(NULL) {
  uint32_t flagbits = 0;
  bool is_global = false;
  BOOST_FOREACH(const UChar& c, flags) {
    if (c == 'g') {
      if (is_global) {
        status_ = U_REGEX_RULE_SYNTAX;
        break;
      } else {
        is_global = true;
      }
    } else if (c == 'm') {
      if (flagbits & UREGEX_MULTILINE) {
        status_ = U_REGEX_RULE_SYNTAX;
        break;
      } else {
        flagbits |= UREGEX_MULTILINE;
      }
    } else if (c == 'i') {
      if (flagbits & UREGEX_CASE_INSENSITIVE) {
        status_ = U_REGEX_RULE_SYNTAX;
        break;
      } else {
        flagbits |= UREGEX_CASE_INSENSITIVE;
      }
    } else {
      status_ = U_REGEX_RULE_SYNTAX;
      break;
    }
  }
  if (status_ == U_ZERO_ERROR) {
    impl_ = new JSRegExpImpl(value, flagbits, is_global, &status_);
  }
}

JSRegExp::JSRegExpImpl::JSRegExpImpl(const core::UStringPiece& value,
                                     uint32_t flags,
                                     bool is_global, UErrorCode* status)
  : regexp_(NULL),
    global_(is_global) {
  UParseError pstatus;
  regexp_ = uregex_open(value.data(), value.size(), flags, &pstatus, status);
}

JSRegExp::JSRegExpImpl::~JSRegExpImpl() {
  uregex_close(regexp_);
}

JSRegExp* JSRegExp::New(const core::UStringPiece& value,
                        const core::UStringPiece& flags) {
  return new JSRegExp(value, flags);
}

} }  // namespace iv::lv5
