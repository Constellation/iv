// FixedStringBuilder is inspired from V8 StringBuilder
// New BSD License
#ifndef IV_FIXED_STRING_BUILDER_H_
#define IV_FIXED_STRING_BUILDER_H_
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>
#include <iv/string_view.h>
#include <iv/fixed_container.h>
namespace iv {
namespace core {

template<class CharT>
class BasicFixedStringBuilder {
 public:
  typedef BasicFixedStringBuilder<CharT> this_type;
  typedef FixedContainer<CharT> container_type;

  typedef typename container_type::value_type value_type;
  typedef typename container_type::pointer pointer;
  typedef typename container_type::const_pointer const_pointer;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::reference reference;
  typedef typename container_type::const_reference const_reference;
  typedef typename container_type::reverse_iterator reverse_iterator;
  typedef typename container_type::const_reverse_iterator const_reverse_iterator;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::difference_type difference_type;

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

  void Append(const u16string_view& piece) {
    Append(piece.begin(), piece.size());
  }

  void Append(const string_view& piece) {
    Append(piece.begin(), piece.size());
  }

  void Append(CharT ch) {
    AddCharacter(ch);
  }

  template<typename Iter>
  void Append(Iter it, typename container_type::size_type size) {
    assert(!IsFinalized() && (point_ + size) < buffer_.size());
    std::copy(it, it + size, buffer_.begin() + point_);
    point_ += size;
  }

  template<typename Iter>
  void Append(Iter start, Iter last) {
    Append(start, std::distance(start, last));
  }

  void AddString(const basic_string_view<CharT>& piece) {
    Append(piece);
  }

  void AddSubstring(const CharT* s, std::size_t n) {
    Append(s, n);
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

  void AddCharacter(CharT ch) {
    push_back(ch);
  }

  void push_back(CharT ch) {
    assert(ch != '\0');
    assert(!IsFinalized() && (point_ + 1) < buffer_.size());
    buffer_[point_++] = ch;
  }

  void clear() {
    point_ = 0;
  }

  basic_string_view<CharT> BuildPiece() const {
    const std::size_t size = point_;
    Finalize();
    return basic_string_view<CharT>(buffer_.data(), size);
  }

  std::basic_string<CharT> Build() const {
    const std::size_t size = point_;
    Finalize();
    return std::basic_string<CharT>(buffer_.data(), size);
  }

  bool IsFinalized() const {
    return point_ == std::string::npos;
  }

 private:

  FixedContainer<CharT> buffer_;
  std::size_t point_;
};

typedef BasicFixedStringBuilder<char> FixedStringBuilder;

} }  // namespace iv::core
#endif  // IV_FIXED_STRING_BUILDER_H_
