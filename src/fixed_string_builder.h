// FixedStringBuilder is inspired from V8 StringBuilder
// New BSD License
#ifndef IV_FIXED_STRING_BUILDER_H_
#define IV_FIXED_STRING_BUILDER_H_
#include <cassert>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include "stringpiece.h"
#include "fixed_container.h"
namespace iv {
namespace core {

class FixedStringBuilder {
 public:
  FixedStringBuilder(char* ptr, std::size_t size)
    : buffer_(ptr, size),
      point_(0) { }

  ~FixedStringBuilder() {
    if (!IsFinalized()) {
      Finalize();
    }
  }

  void AddCharacter(char ch) {
    assert(ch != '\0');
    assert(!IsFinalized() && static_cast<std::size_t>(point_) < buffer_.size());
    buffer_[point_++] = ch;
  }

  void AddString(const StringPiece& piece) {
    AddSubstring(piece.data(), piece.size());
  }

  void AddSubstring(const char* s, int n) {
    assert(!IsFinalized() &&
           static_cast<std::size_t>(point_ + n) < buffer_.size());
    assert(static_cast<std::size_t>(n) <= std::strlen(s));
    std::copy(s, s + n, buffer_.begin() + point_);
    point_ += n;
  }

  void AddInteger(int n) {
    assert(IsFinalized() && static_cast<std::size_t>(point_) < buffer_.size());
    int ndigits = 0;
    int n2 = n;
    do {
      ++ndigits;
      n2 /= 10;
    } while (n2);
    point_ += ndigits;
    int i = point_;
    do {
      buffer_[--i] = '0' + (n % 10);
      n /= 10;
    } while (n);
  }

  void AddPading(char c, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
      AddCharacter(c);
    }
  }

  char* Finalize() {
    buffer_[point_] = '\0';
    return buffer_.data();
  }

 private:
  bool IsFinalized() const {
    return point_ < 0;
  }

  FixedContainer<char> buffer_;
  int point_;
};

} }  // namespace iv::core
#endif  // IV_FIXED_STRING_BUILDER_H_
