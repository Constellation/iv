#ifndef _IV_LV5_REGEXP_H_
#define _IV_LV5_REGEXP_H_
#include <cstdlib>
#include <cstring>
#include <memory>
#include <gc/gc_cpp.h>
#include <iv/string_view.h>
#include <iv/space.h>
#include <iv/aero/flags.h>
#include <iv/lv5/context.h>
namespace iv {

namespace aero {
class Code;
}  // namespace aero

namespace lv5 {

class RegExp : public gc_cleanup {
 public:
  enum Flags {
    NONE = 0,
    GLOBAL = 1,
    IGNORE_CASE = 2,
    MULTILINE = 4,
    STICKY = 8
  };

  explicit RegExp(core::Space* allocator);

  ~RegExp();

  RegExp(core::Space* allocator, const core::u16string_view& value, int flags);

  RegExp(core::Space* allocator, const core::string_view& value, int flags);

  bool IsValid() const { return !!code_; }

  bool global() const { return flags_ & GLOBAL; }

  bool ignore() const { return flags_ & IGNORE_CASE; }

  bool multiline() const { return flags_ & MULTILINE; }

  bool sticky() const { return flags_ & STICKY; }

  int number_of_captures() const;

  template<typename Iter>
  static int ComputeFlags(Iter it, Iter last) {
    int flags = 0;
    for (; it != last; ++it) {
      const char16_t c = *it;
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
      } else if (c == 'y') {
        if (flags & STICKY) {
          return -1;
        } else {
          // currently not supported
          // flags |= STICKY;
          return -1;
        }
      } else {
        return -1;
      }
    }
    return flags;
  }

  int Execute(Context* ctx, core::string_view subject,
              int offset, int* offset_vector) const;

  int Execute(Context* ctx, core::u16string_view subject,
              int offset, int* offset_vector) const;

 private:
  template<typename Source>
  void Initialize(core::Space* allocator, const Source& value);

  int flags_;
  int error_;
  std::unique_ptr<aero::Code> code_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_REGEXP_H_
