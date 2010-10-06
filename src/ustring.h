#ifndef _IV_USTRING_H_
#define _IV_USTRING_H_
#include <string>
#include <functional>
#include <numeric>
#include <cstring>
#include <vector>
#include <tr1/unordered_map>
#include <tr1/functional>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include "stringpiece.h"
namespace iv {
namespace core {

typedef std::basic_string<UChar,
                          std::char_traits<UChar> > UString;

template<typename Out>
inline void ConvertToUTF16(StringPiece str, Out* out,
                           const char * code = "utf-8") {
  UErrorCode error = U_ZERO_ERROR;
  u_init(&error);
  UConverter* const conv = ucnv_open(code, &error);
  if (U_FAILURE(error)) {
    out->clear();
    return;
  } else {
    const std::size_t source_length = str.size() + 1;
    const std::size_t target_length = source_length / ucnv_getMinCharSize(conv);
    const char* pointer = str.data();
    std::vector<UChar> vec(target_length + 1);
    UChar* utarget = vec.data();
    ucnv_toUnicode(conv, &utarget, utarget+target_length,
                   &pointer, str.data()+source_length, NULL, true, &error);
    if (U_FAILURE(error)) {
      out->clear();
    } else {
      out->assign(vec.data(), utarget-1);
    }
  }
  ucnv_close(conv);
  u_cleanup();
}

} }  // namespace iv::core

namespace std {
namespace tr1 {

// template specialization for UString in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::core::UString>
  : public unary_function<iv::core::UString, std::size_t> {
  std::size_t operator()(const iv::core::UString& x) const {
    return std::accumulate(
        x.begin(),
        x.end(),
        0,
        std::tr1::bind(
            std::plus<std::size_t>(),
            std::tr1::bind(
                std::multiplies<std::size_t>(),
                std::tr1::placeholders::_1,
                131),
            std::tr1::placeholders::_2));
  }
};

} }  // namespace std::tr1
#endif  // _IV_USTRING_H_
