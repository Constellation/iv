#ifndef _IV_ICU_USTRING_H_
#define _IV_ICU_USTRING_H_
#include <cstring>
#include <vector>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include "uchar.h"
#include "ustring.h"
#include "stringpiece.h"
#include "conversions.h"
namespace iv {
namespace icu {

template<typename Out>
inline void ConvertToUTF16(const core::StringPiece& str,
                           Out* out,
                           const core::StringPiece code = "utf-8") {
  UErrorCode error = U_ZERO_ERROR;
  u_init(&error);
  UConverter* const conv = ucnv_open(code.data(), &error);
  if (U_FAILURE(error)) {
    out->clear();
    return;
  } else {
    const std::size_t source_length = str.size() + 1;
    const std::size_t target_length = source_length / ucnv_getMinCharSize(conv);
    const char* pointer = str.data();
    std::vector<uc16> vec(target_length + 1);
    uc16* utarget = vec.data();
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

} }  // namespace iv::icu
#endif  // _IV_ICU_USTRING_H_
