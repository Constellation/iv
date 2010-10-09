#include <cstring>
#include <vector>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <algorithm>
#include "jsstring.h"

namespace iv {
namespace lv5 {

JSString::JSString()
  : GCUString(),
    hash_value_(CalcHash(begin(), end())) {
}

JSString::JSString(size_type len, UChar ch)
  : GCUString(len, ch),
    hash_value_(CalcHash(begin(), end())) {
}

JSString::JSString(const UChar* s)
  : GCUString(s),
    hash_value_(CalcHash(begin(), end())) {
}

JSString::JSString(const UChar* s, size_type len)
  : GCUString(s, len),
    hash_value_(CalcHash(begin(), end())) {
}

JSString::JSString(const GCUString& s, size_type index, size_type len)
  : GCUString(s, index, len),
    hash_value_(CalcHash(begin(), end())) {
}

JSString::JSString(const JSString& str)
  : GCUString(str.begin(), str.end()),
    hash_value_(str.hash_value_) {
}

JSString* JSString::New(Context* ctx, const core::StringPiece& str) {
  JSString* res;
  UErrorCode error = U_ZERO_ERROR;
  UConverter* conv;
  UChar* utarget;
  u_init(&error);
  conv = ucnv_open("utf-8", &error);
  if (U_FAILURE(error)) {
    // Null String
    res = new JSString();
  } else {
    const std::size_t source_length = str.size() + 1;
    const std::size_t target_length = source_length / ucnv_getMinCharSize(conv);
    const char* pointer = str.data();
    std::vector<UChar> vec(target_length + 1);
    utarget = vec.data();
    ucnv_toUnicode(conv, &utarget, utarget+target_length,
                   &pointer, str.data()+source_length, NULL, true, &error);
    if (U_FAILURE(error)) {
      res = new JSString();
    } else {
      *utarget = '\0';
      res = new JSString(vec.data(), std::distance(vec.data(), utarget));
    }
  }
  ucnv_close(conv);
  u_cleanup();
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
