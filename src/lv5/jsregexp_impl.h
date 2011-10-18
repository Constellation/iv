#ifndef IV_LV5_JSREGEXP_IMPL_H_
#define IV_LV5_JSREGEXP_IMPL_H_
#include <cstdlib>
#include <cstring>
#include <gc/gc_cpp.h>
#include "ustring.h"
#include "ustringpiece.h"
#include "space.h"
#include "lv5/context.h"
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
  JSRegExpImpl(core::Space* allocator,
               const core::UStringPiece& value,
               const core::UStringPiece& flags)
    : flags_(NONE),
      error_(0),
      code_(NULL) {
    Initialize(allocator, value, flags);
  }

  JSRegExpImpl(core::Space* allocator)
    : flags_(NONE),
      error_(0),
      code_(NULL) {
    Initialize(allocator, detail::kEmptyPattern, core::UStringPiece());
  }

  ~JSRegExpImpl() {
    if (code_) {
      delete code_;
    }
  }

  bool IsValid() const {
    return code_;
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

  int number_of_captures() const {
    assert(IsValid());
    return code_->captures();
  }

  int ExecuteOnce(Context* ctx,
                  const core::UStringPiece& subject,
                  int offset,
                  int* offset_vector) const {
    assert(IsValid());
    return ctx->regexp_vm()->ExecuteOnce(code_, subject, offset, offset_vector);
  }

 private:
  void Initialize(core::Space* allocator,
                  const core::UStringPiece& value,
                  const core::UStringPiece& flags) {
    bool out = false;
    int f = aero::NONE;
    for (core::UStringPiece::const_iterator it = flags.begin(),
         last = flags.end(); it != last; ++it) {
      const uint16_t c = *it;
      if (c == 'g') {
        if (global()) {
          out = true;
          break;
        } else {
          flags_ |= GLOBAL;
        }
      } else if (c == 'm') {
        if (aero::MULTILINE & f) {
          out = true;
          break;
        } else {
          f |= aero::MULTILINE;
          flags_ |= MULTILINE;
        }
      } else if (c == 'i') {
        if (aero::IGNORE_CASE & f) {
          out = true;
          break;
        } else {
          f |= aero::IGNORE_CASE;
          flags_ |= IGNORECASE;
        }
      } else {
        out = true;
        break;
      }
    }
    if (!out) {
      code_ = aero::Compile(allocator, value, f, &error_);
    }
  }

  int flags_;
  int error_;
  aero::Code* code_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSREGEXP_IMPL_H_
