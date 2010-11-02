#ifndef _IV_SOURCE_H_
#define _IV_SOURCE_H_

#include <cstddef>
#include <cassert>
#include <string>
#include "ustring.h"
#include "ustringpiece.h"

namespace iv {
namespace core {

class Source {
 public:
  static const int kEOS = -1;
  Source(const std::string& str, const std::string& filename)
    : source_(),
      filename_(filename) {
    ParseMagicComment(str);
  }

  inline UChar Get(std::size_t pos) const {
    assert(pos < size());
    return source_[pos];
  }
  inline std::size_t size() const {
    return source_.size();
  }
  inline const std::string& filename() const {
    return filename_;
  }
  inline const UString& source() const {
    return source_;
  }
  inline UStringPiece SubString(std::size_t n,
                                std::size_t len = std::string::npos) const {
    if (len == std::string::npos) {
      return UStringPiece((source_.data() + n), (source_.size() - n));
    } else {
      return UStringPiece((source_.data() + n), len);
    }
  }

 private:

  int inline Next(const char* begin, const char* end) {
    return (begin == end) ? kEOS : *begin;
  }

  void ParseMagicComment(const std::string& str) {
    const char* begin = str.c_str();
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
      if (Chars::IsLineTerminator(ch)) {
        ch = Next(current, end);
        if (Chars::IsLineTerminator(ch)) {
          ++current;
        }
        return current;
      }
      ch = Next(current, end);
      ++current;
    }
    return current;
  }

  UString source_;
  std::string filename_;
};

} }  // namespace iv::core
#endif  // _IV_SOURCE_H_
