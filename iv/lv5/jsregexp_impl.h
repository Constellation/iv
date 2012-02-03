#ifndef IV_LV5_JSREGEXP_IMPL_H_
#define IV_LV5_JSREGEXP_IMPL_H_
#include <cstdlib>
#include <cstring>
#include <gc/gc_cpp.h>
#include <iv/ustring.h>
#include <iv/ustringpiece.h>
#include <iv/space.h>
#include <iv/aero/aero.h>
#include <iv/lv5/context.h>

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
    IGNORE_CASE = 2,
    MULTILINE = 4
  };

  JSRegExpImpl(core::Space* allocator,
               const core::UStringPiece& value, int flags)
    : flags_(flags),
      error_(0),
      code_(NULL) {
    Initialize(allocator, value);
  }

  JSRegExpImpl(core::Space* allocator,
               const core::StringPiece& value, int flags)
    : flags_(flags),
      error_(0),
      code_(NULL) {
    Initialize(allocator, value);
  }

  JSRegExpImpl(core::Space* allocator)
    : flags_(NONE),
      error_(0),
      code_(NULL) {
    Initialize(allocator, detail::kEmptyPattern);
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
    return flags_ & IGNORE_CASE;
  }

  bool multiline() const {
    return flags_ & MULTILINE;
  }

  int number_of_captures() const {
    assert(IsValid());
    return code_->captures();
  }

  template<typename Iter>
  static int ComputeFlags(Iter it, Iter last) {
    int flags = 0;
    for (; it != last; ++it) {
      const uint16_t c = *it;
      if (c == 'g') {
        if (flags & GLOBAL) {
          return -1;
        } else {
          flags |= GLOBAL;
        }
      } else if (c == 'm') {
        if (flags & MULTILINE) {
          return -1;
        } else {
          flags |= MULTILINE;
        }
      } else if (c == 'i') {
        if (flags & IGNORE_CASE) {
          return -1;
        } else {
          flags |= IGNORE_CASE;
        }
      } else {
        return -1;
      }
    }
    return flags;
  }

  template<typename T>
  int Execute(Context* ctx,
              const T& subject,
              int offset,
              int* offset_vector) const {
    assert(IsValid());
    return ctx->regexp_vm()->Execute(code_, subject, offset_vector, offset);
  }

 private:
  template<typename Source>
  void Initialize(core::Space* allocator, const Source& value) {
    if (flags_ != -1) {
      const int f =
          ((flags_ & MULTILINE) ? aero::MULTILINE : aero::NONE) |
          ((flags_ & IGNORE_CASE) ? aero::IGNORE_CASE : aero::NONE);
      code_ = aero::Compile(allocator, value, f, &error_);
    }
  }

  int flags_;
  int error_;
  aero::Code* code_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSREGEXP_IMPL_H_
