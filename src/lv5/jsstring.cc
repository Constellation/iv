#include <cstring>
#include <vector>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <algorithm>
#include "jsstring.h"
#include "ustring.h"

namespace iv {
namespace lv5 {

JSString::JSString()
  : GCUString(),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(size_type len, UChar ch)
  : GCUString(len, ch),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(const UChar* s)
  : GCUString(s),
    hash_value_(core::StringToHash(*this)) {
}

JSString::JSString(const UChar* s, size_type len)
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
  core::ConvertToUTF16(str, res);
  return res;
}

JSString* JSString::New(Context* ctx, const core::UStringPiece& str) {
  return new JSString(str.data(), str.size());
}

JSString* JSString::NewAsciiString(Context* ctx,
                                   const core::StringPiece& str) {
  return new JSString(str.begin(), str.end());
}

} }  // namespace iv::lv5
