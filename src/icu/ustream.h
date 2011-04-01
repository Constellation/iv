#ifndef _IV_ICU_USTREAM_H_
#define _IV_ICU_USTREAM_H_
#include <iostream>  // NOLINT
#include <vector>
#include <iterator>
#include <algorithm>
#include <tr1/array>
#include <tr1/type_traits>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/uchar.h>
#include "uchar.h"
#include "ustringpiece.h"

namespace iv {
namespace icu {
namespace detail {

template<bool UCharIsUCS2>
struct Convert { };

template<>
struct Convert<true> {
  static void Output(std::ostream& os, const core::UStringPiece& str) {
    std::tr1::array<char, 200> buffer;
    UErrorCode error = U_ZERO_ERROR;
    UConverter* conv = ucnv_open(NULL, &error);
    if (U_FAILURE(error)) {
      ucnv_close(conv);
    } else {
      const UChar* start = str.data();
      const UChar* limit = start + str.size();
      char* s;
      const char* slimit = buffer.data() + buffer.size();
      do {
        error = U_ZERO_ERROR;
        s = buffer.data();
        ucnv_fromUnicode(conv, &s, slimit, &start, limit, 0, false, &error);
        if (s > buffer.data()) {
          os.write(buffer.data(), std::distance(buffer.data(), s));
        }
      } while (error == U_BUFFER_OVERFLOW_ERROR);
      ucnv_close(conv);
    }
  }
};

template<>
struct Convert<false> {
  static void Output(std::ostream& os, const core::UStringPiece& str) {
    std::tr1::array<char, 200> buffer;
    UErrorCode error = U_ZERO_ERROR;
    UConverter* conv = ucnv_open(NULL, &error);
    if (U_FAILURE(error)) {
      ucnv_close(conv);
    } else {
      std::vector<UChar> vec;
      vec.reserve(str.size());
      std::copy(str.begin(), str.end(), std::back_inserter(vec));
      const UChar* start = vec.data();
      const UChar* limit = start + vec.size();
      char* s;
      const char* slimit = buffer.data() + buffer.size();
      do {
        error = U_ZERO_ERROR;
        s = buffer.data();
        ucnv_fromUnicode(conv, &s, slimit, &start, limit, 0, false, &error);
        if (s > buffer.data()) {
          os.write(buffer.data(), std::distance(buffer.data(), s));
        }
      } while (error == U_BUFFER_OVERFLOW_ERROR);
      ucnv_close(conv);
    }
  }
};

} } }  // namespace iv::icu::detail

inline std::ostream& operator<<(std::ostream& os,
                                const iv::core::UStringPiece& str) {
  if (!str.empty()) {
    iv::icu::detail::Convert<
        std::tr1::is_same<UChar, iv::uc16>::value >::Output(os, str);
  }
  return os;
}

#endif  // _IV_ICU_USTREAM_H_
