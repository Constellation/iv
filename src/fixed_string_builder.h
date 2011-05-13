// FixedStringBuilder is inspired from V8 StringBuilder
// New BSD License
#ifndef _IV_FIXED_STRING_BUILDER_H_
#define _IV_FIXED_STRING_BUILDER_H_
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>
#include "stringpiece.h"
#include "fixed_container.h"
namespace iv {
namespace core {

template<class CharT>
class BasicFixedStringBuilder {
 public:
  BasicFixedStringBuilder(CharT* ptr, std::size_t size)
    : buffer_(ptr, size),
      point_(0) {
    // fill in destructor, at least, size should be more than 0.
    assert(size != 0);
  }

  ~BasicFixedStringBuilder() {
    if (!IsFinalized()) {
      Finalize();
    }
  }

  void AddCharacter(CharT ch) {
    assert(ch != '\0');
    assert(!IsFinalized() && (point_ + 1) < buffer_.size());
    buffer_[point_++] = ch;
  }

  void AddString(const BasicStringPiece<CharT>& piece) {
    AddSubstring(piece.data(), piece.size());
  }

  void AddSubstring(const CharT* s, std::size_t n) {
    assert(!IsFinalized() && (point_ + n) < buffer_.size());
    std::copy(s, s + n, buffer_.begin() + point_);
    point_ += n;
  }

  void AddInteger(int n) {
    assert(!IsFinalized() && point_ < buffer_.size());
    int ndigits = 0;
    int n2 = n;
    do {
      ++ndigits;
      n2 /= 10;
    } while (n2);
    point_ += ndigits;
    std::size_t i = point_;
    assert(!IsFinalized() && (point_ + ndigits) < buffer_.size());
    do {
      buffer_[--i] = '0' + (n % 10);
      n /= 10;
    } while (n);
  }

  void AddPading(CharT c, std::size_t count) {
    assert(!IsFinalized() && (point_ + count) < buffer_.size());
    std::fill_n(buffer_.begin() + point_, count, c);
    point_ += count;
  }

  CharT* Finalize() {
    assert(!IsFinalized() && (point_ + 1) < buffer_.size());
    buffer_[point_] = '\0';
    point_ = std::string::npos;
    return buffer_.data();
  }

 private:
  bool IsFinalized() const {
    return point_ == std::string::npos;
  }

  FixedContainer<CharT> buffer_;
  std::size_t point_;
};

typedef BasicFixedStringBuilder<char> FixedStringBuilder;

} }  // namespace iv::core
#endif  // _IV_FIXED_STRING_BUILDER_H_
