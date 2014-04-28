#include <cstdlib>
#include <cstring>
#include <iv/ustring.h>
#include <iv/string_view.h>
#include <iv/space.h>
#include <iv/aero/aero.h>
#include <iv/lv5/context.h>
#include <iv/lv5/regexp.h>
#include <iv/lv5/property.h>
namespace iv {
namespace lv5 {

static const std::u16string kEmptyPattern = core::ToU16String("(?:)");

RegExp::RegExp(core::Space* allocator,
               const core::u16string_view& value, int flags)
  : flags_(flags),
    error_(0),
    code_() {
  Initialize(allocator, value);
}

RegExp::RegExp(core::Space* allocator,
               const core::string_view& value, int flags)
  : flags_(flags),
    error_(0),
    code_() {
  Initialize(allocator, value);
}

RegExp::RegExp(core::Space* allocator)
  : flags_(NONE),
    error_(0),
    code_() {
  Initialize(allocator, kEmptyPattern);
}

RegExp::~RegExp() { }

int RegExp::number_of_captures() const {
  assert(IsValid());
  return code_->captures();
}

int RegExp::Execute(Context* ctx,
                    core::string_view subject,
                    int offset, int* offset_vector) const {
  assert(IsValid());
  return ctx->regexp_vm()->Execute(code_.get(), subject, offset_vector, offset);
}

int RegExp::Execute(Context* ctx,
                    core::u16string_view subject,
                    int offset, int* offset_vector) const {
  assert(IsValid());
  return ctx->regexp_vm()->Execute(code_.get(), subject, offset_vector, offset);
}

template<typename Source>
void RegExp::Initialize(core::Space* allocator, const Source& value) {
  if (flags_ != -1) {
    const int f =
        ((flags_ & MULTILINE) ? aero::MULTILINE : aero::NONE) |
        ((flags_ & IGNORE_CASE) ? aero::IGNORE_CASE : aero::NONE);
    code_.reset(aero::Compile(allocator, value, f, &error_));
  }
}

} }  // namespace iv::lv5
