#ifndef IV_LV5_JSREGEXP_IMPL_H_
#define IV_LV5_JSREGEXP_IMPL_H_
#include <cstdlib>
#include <cstring>
#include <gc/gc_cpp.h>
#include "ustring.h"
#include "ustringpiece.h"
#include "space.h"

#ifdef DEBUG
#include "lv5/third_party/jscre/pcre.h"
#define DEBUG
#else
#include "lv5/third_party/jscre/pcre.h"
#endif

#include "lv5/aero/aero.h"

namespace iv {
namespace lv5 {
namespace detail {

static const core::UString kEmptyPattern = core::ToUString("(?:)");

}  // namespace detail

class JSRegExpImpl : public gc_cleanup {
 public:
  enum Flags {
    NONE = 0,
    GLOBAL = 1,
    IGNORECASE = 2,
    MULTILINE = 4
  };
  JSRegExpImpl(core::Space* space,
               const core::UStringPiece& value,
               const core::UStringPiece& flags)
    : reg_(NULL),
      number_of_captures_(0),
      flags_(NONE),
      error_(NULL),
      code_(NULL) {
    Initialize(space, value, flags);
  }

  JSRegExpImpl(core::Space* space)
    : reg_(NULL),
      number_of_captures_(0),
      flags_(NONE),
      error_(NULL),
      code_(NULL) {
    Initialize(space, detail::kEmptyPattern, core::UStringPiece());
  }

  ~JSRegExpImpl() {
    if (reg_) {
      jscre::jsRegExpFree(reg_);
    }
    if (code_) {
      delete code_;
    }
  }

  bool IsValid() const {
    return reg_;
  }

  const char* Error() const {
    return error_;
  }

  bool global() const {
    return flags_ & GLOBAL;
  }

  bool ignore() const {
    return flags_ & IGNORECASE;
  }

  bool multiline() const {
    return flags_ & MULTILINE;
  }

  uint32_t number_of_captures() const {
    return number_of_captures_;
  }

  template<typename String, typename T>
  int ExecuteOnce(const String& subject,
                  int offset,
                  T* offset_vector) const {
    return jscre::jsRegExpExecute(reg_,
                                  subject.data(), subject.size(),
                                  offset,
                                  offset_vector->data(),
                                  offset_vector->size());
  }

 private:
  void Initialize(core::Space* space,
                  const core::UStringPiece& value,
                  const core::UStringPiece& flags) {
    bool state = false;
    jscre::JSRegExpIgnoreCaseOption ignore = jscre::JSRegExpDoNotIgnoreCase;
    jscre::JSRegExpMultilineOption multi = jscre::JSRegExpSingleLine;
    for (core::UStringPiece::const_iterator it = flags.begin(),
         last = flags.end(); it != last; ++it) {
      const uint16_t c = *it;
      if (c == 'g') {
        if (global()) {
          state = true;
          break;
        } else {
          flags_ |= GLOBAL;
        }
      } else if (c == 'm') {
        if (multi == jscre::JSRegExpMultiline) {
          state = true;
          break;
        } else {
          multi = jscre::JSRegExpMultiline;
          flags_ |= MULTILINE;
        }
      } else if (c == 'i') {
        if (ignore == jscre::JSRegExpIgnoreCase) {
          state = true;
          break;
        } else {
          ignore = jscre::JSRegExpIgnoreCase;
          flags_ |= IGNORECASE;
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
                                    &error_);
      const int flags =
          (ignore ? aero::IGNORE_CASE : 0) | (multi ? aero::MULTILINE : 0);
      int error = 0;
      code_ = aero::Compile(space, value, flags, &error);
    }
  }

  jscre::JSRegExp* reg_;
  uint32_t number_of_captures_;
  int flags_;
  const char* error_;
  aero::Code* code_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSREGEXP_IMPL_H_
