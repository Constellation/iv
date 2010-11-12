#include <boost/foreach.hpp>
#include "jsregexp-impl.h"
namespace iv {
namespace lv5 {

JSRegExpImpl::JSRegExpImpl(const core::UStringPiece& value,
                           const core::UStringPiece& flags,
                           UErrorCode* status)
  : regexp_(NULL),
    global_(false) {
  Initialize(value, flags, status);
}

JSRegExpImpl::JSRegExpImpl()
  : regexp_(NULL),
    global_(false) {
}

void JSRegExpImpl::Initialize(const core::UStringPiece& value,
                              const core::UStringPiece& flags,
                              UErrorCode* status) {
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
  if (regexp_) {
    uregex_close(regexp_);
  }
}

} }  // namespace iv::lv5
