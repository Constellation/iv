#ifndef _IV_LV5_JSREGEXP_IMPL_H_
#define _IV_LV5_JSREGEXP_IMPL_H_
#include <cstdlib>
#include <gc/gc_cpp.h>
#include "ustring.h"
#include "ustringpiece.h"

#ifdef DEBUG
#include "third_party/jscre/pcre.h"
#define DEBUG
#else
#include "third_party/jscre/pcre.h"
#endif

namespace iv {
namespace lv5 {
class JSRegExpImpl : public gc_cleanup {
 public:
  JSRegExpImpl(const core::UStringPiece& value,
               const core::UStringPiece& flags)
    : reg_(NULL),
      str_(value),
      number_of_captures_(0),
      global_(false),
      error_(NULL) {
    Initialize(value, flags);
  }

  void Initialize(const core::UStringPiece& value,
                  const core::UStringPiece& flags) {
    bool state = false;
    jscre::JSRegExpIgnoreCaseOption ignore = jscre::JSRegExpDoNotIgnoreCase;
    jscre::JSRegExpMultilineOption multi = jscre::JSRegExpSingleLine;
    for (core::UStringPiece::const_iterator it = flags.begin(),
         last = flags.end(); it != last; ++it) {
      const uc16 c = *it;
      if (c == 'g') {
        if (global_) {
          state = true;
          break;
        } else {
          global_ = true;
        }
      } else if (c == 'm') {
        if (multi == jscre::JSRegExpMultiline) {
          state = true;
          break;
        } else {
          multi = jscre::JSRegExpMultiline;
        }
      } else if (c == 'i') {
        if (ignore == jscre::JSRegExpIgnoreCase) {
          state = true;
          break;
        } else {
          ignore = jscre::JSRegExpIgnoreCase;
        }
      } else {
        state = true;
        break;
      }
    }
    if (!state) {
      reg_ = jscre::jsRegExpCompile(value.data(), value.size(),
                                    ignore,
                                    multi,
                                    &number_of_captures_,
                                    &error_,
                                    std::malloc,
                                    std::free);
    }
  }

  ~JSRegExpImpl() {
    if (reg_) {
      jscre::jsRegExpFree(reg_, std::free);
    }
  }

  bool IsValid() const {
    return reg_;
  }

  const char* Error() const {
    return error_;
  }

 private:
  jscre::JSRegExp* reg_;
  core::UStringPiece str_;
  uint32_t number_of_captures_;
  bool global_;
  const char* error_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_IMPL_H_
