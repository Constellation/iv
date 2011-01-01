#ifndef _IV_ICU_SOURCE_H_
#define _IV_ICU_SOURCE_H_
#include <cstddef>
#include <cassert>
#include <string>
#include "chars.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "icu/uconv.h"

namespace iv {
namespace icu {

class Source {
 public:
  static const int kEOS = -1;
  Source(const core::StringPiece& str,
         const core::StringPiece& filename)
    : source_(),
      filename_(filename.data(), filename.size()) {
    ParseMagicComment(str);
  }

  inline uc16 Get(std::size_t pos) const {
    assert(pos < size());
    return source_[pos];
  }
  inline std::size_t size() const {
    return source_.size();
  }
  inline const std::string& filename() const {
    return filename_;
  }
  inline core::UStringPiece SubString(
      std::size_t n, std::size_t len = std::string::npos) const {
    if (len == std::string::npos) {
      return core::UStringPiece((source_.data() + n), (source_.size() - n));
    } else {
      return core::UStringPiece((source_.data() + n), len);
    }
  }

 private:

  int inline Next(const char* begin, const char* end) {
    return (begin == end) ? kEOS : *begin;
  }

  void ParseMagicComment(const core::StringPiece& str) {
    const char* begin = str.data();
    const char* end   = begin + str.size();

    const char* current = begin;

    // parse shebang
    int ch = Next(current, end);
    ++current;
    if (ch == '#') {
      ch = Next(current, end);
      ++current;
      if (ch == '!') {
        // shebang as SingleLineComment
        current = SkipSingleLineComment(current, end);
        ch = Next(current, end);
        ++current;
      } else {
        // not valid shebang
        ConvertToUTF16(str, &source_, "utf-8");
        return;
      }

      // magic comment
      if (ch == '/') {
        ch = Next(current, end);
        ++current;
        if (ch == '/' || ch == '*') {
          // comment
          ConvertToUTF16(str, &source_, "utf-8");
          return;
        }
      }
    }
    ConvertToUTF16(str, &source_, "utf-8");
  }

  const char* SkipSingleLineComment(const char* current,
                                    const char* end) {
    int ch = Next(current, end);
    ++current;
    while (ch >= 0) {
      if (core::Chars::IsLineTerminator(ch)) {
        ch = Next(current, end);
        if (ch >= 0 && core::Chars::IsLineTerminator(ch)) {
          ++current;
        }
        return current;
      }
      ch = Next(current, end);
      ++current;
    }
    return current;
  }

  core::UString source_;
  std::string filename_;
};

} }  // namespace iv::icu
#endif  // _IV_ICU_SOURCE_H_
