#ifndef _IV_LV5_JSREGEXP_IMPL_H_
#define _IV_LV5_JSREGEXP_IMPL_H_
#include <unicode/regex.h>
#include <gc/gc_cpp.h>
#include "ustring.h"
#include "ustringpiece.h"

namespace iv {
namespace lv5 {
class JSRegExpImpl : public gc_cleanup {
 public:
  JSRegExpImpl(const core::UStringPiece& value,
               const core::UStringPiece& flags,
               UErrorCode* status)
    : regexp_(NULL),
      global_(false) {
    Initialize(value, flags, status);
  }

  JSRegExpImpl()
    : regexp_(NULL),
      global_(false) {
  }

  void Initialize(const core::UStringPiece& value,
                  const core::UStringPiece& flags,
                  UErrorCode* status) {
    uint32_t flagbits = 0;
    for (core::UStringPiece::const_iterator it = flags.begin(),
         last = flags.end(); it != last; ++it) {
      const uc16 c = *it;
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

  ~JSRegExpImpl() {
    if (regexp_) {
      uregex_close(regexp_);
    }
  }

 private:
  URegularExpression* regexp_;
  bool global_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_IMPL_H_
