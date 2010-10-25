#include "source.h"
#include "ctype.h"

namespace iv {
namespace core {

Source::Source(const std::string& str, const std::string& filename)
  : source_(),
    filename_(filename) {
  ParseMagicComment(str);
}

void Source::ParseMagicComment(const std::string& str) {
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

const char* Source::SkipSingleLineComment(const char* current,
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

} }  // namespace iv::core
