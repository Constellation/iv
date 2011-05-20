#ifndef _IV_FILE_SOURCE_H_
#define _IV_FILE_SOURCE_H_
#include <cstddef>
#include <cassert>
#include <string>
#include "unicode.h"
#include "character.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "source_traits.h"
namespace iv {
namespace core {

class FileSource {
 public:
  static const int kEOS = -1;
  FileSource(const core::StringPiece& str,
         const core::StringPiece& filename)
    : source_(),
      filename_(filename.data(), filename.size()) {
    ParseMagicComment(str);
  }

  inline uc16 operator[](std::size_t pos) const {
    assert(pos < size());
    return source_[pos];
  }

  inline std::size_t size() const {
    return source_.size();
  }

  inline const std::string& filename() const {
    return filename_;
  }

  core::UStringPiece GetData() const {
    return source_;
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
        source_.reserve(str.size());
        if (unicode::detail::UTF8ToUTF16(
                str.begin(),
                str.end(),
                std::back_inserter(source_)) != unicode::NO_ERROR) {
          source_.clear();
        }
        return;
      }

      // magic comment
      if (ch == '/') {
        ch = Next(current, end);
        ++current;
        if (ch == '/' || ch == '*') {
          source_.reserve(str.size());
          if (unicode::detail::UTF8ToUTF16(
                  str.begin(),
                  str.end(),
                  std::back_inserter(source_)) != unicode::NO_ERROR) {
            source_.clear();
          }
          return;
        }
      }
    }

    source_.reserve(str.size());
    if (unicode::detail::UTF8ToUTF16(
            str.begin(),
            str.end(),
            std::back_inserter(source_)) != unicode::NO_ERROR) {
      source_.clear();
    }
    return;
  }

  const char* SkipSingleLineComment(const char* current,
                                    const char* end) {
    int ch = Next(current, end);
    ++current;
    while (ch >= 0) {
      if (core::character::IsLineTerminator(ch)) {
        ch = Next(current, end);
        if (ch >= 0 && core::character::IsLineTerminator(ch)) {
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

template<>
struct SourceTraits<FileSource> {
  static std::string GetFileName(const FileSource& src) {
    return src.filename();
  }
};

} }  // namespace iv::core
#endif  // _IV_FILE_SOURCE_H_
