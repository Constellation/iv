#include <cstring>
#include <vector>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <algorithm>
#include "jsstring.h"
#include "ustring.h"
#include "icu/uconv.h"

namespace iv {
namespace lv5 {

JSString::JSString()
  : GCUString(),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(size_type len, uc16 ch)
  : GCUString(len, ch),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(const uc16* s)
  : GCUString(s),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(const uc16* s, size_type len)
  : GCUString(s, len),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(const GCUString& s, size_type index, size_type len)
  : GCUString(s, index, len),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(const JSString& str)
  : GCUString(str.begin(), str.end()),
    hash_value_(str.hash_value_) {
}

JSString* JSString::New(Context* ctx, const core::StringPiece& str) {
  JSString* res = new JSString();
  icu::ConvertToUTF16(str, res);
  return res;
}

JSString* JSString::New(Context* ctx, const core::UStringPiece& str) {
  return new JSString(str.data(), str.size());
}

JSString* JSString::NewAsciiString(Context* ctx,
                                   const core::StringPiece& str) {
  return new JSString(str.begin(), str.end());
}

JSString* JSString::NewEmptyString(Context* ctx) {
  return new JSString();
}

} }  // namespace iv::lv5
