#ifndef _IV_USTREAM_H_
#define _IV_USTREAM_H_
#include <iosfwd>  // NOLINT
#include <tr1/array>
#include "config.h"

#if USE_ICU == 1
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/uchar.h>

inline std::ostream& operator<<(std::ostream& os, const UChar* str) {
  if (str) {
    std::tr1::array<char, 200> buffer;
    UErrorCode error = U_ZERO_ERROR;
    UConverter* conv = ucnv_open(NULL, &error);
    if (U_FAILURE(error)) {
      ucnv_close(conv);
    } else {
      const UChar* limit = str + std::char_traits<UChar>::length(str);
      char* s;
      const char* slimit = buffer.data() + buffer.size();
      do {
        error = U_ZERO_ERROR;
        s = buffer.data();
        ucnv_fromUnicode(conv, &s, slimit, &str, limit, 0, false, &error);
        if (s > buffer.data()) {
          os.write(buffer.data(), std::distance(buffer.data(), s));
        }
      } while (error == U_BUFFER_OVERFLOW_ERROR);
      ucnv_close(conv);
    }
  }
  return os;
}

#endif  // USE_ICU == 1

#endif  // _IV_USTREAM_H_
