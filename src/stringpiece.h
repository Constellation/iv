#ifndef _IV_STRINGPIECE_H_
#define _IV_STRINGPIECE_H_
#pragma once

#include <algorithm>
#include <iosfwd>
#include <string>
#include <iterator>
#include <limits>

namespace iv {
namespace core {

template<class CharT, class Traits = std::char_traits<CharT> >
class BasicStringPiece {
 public:
  typedef size_t size_type;
  typedef BasicStringPiece<CharT, Traits> this_type;

 private:
  const CharT*   ptr_;
  size_type     length_;

 public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  BasicStringPiece() : ptr_(NULL), length_(0) { }
  BasicStringPiece(const CharT* str)  // NOLINT
    : ptr_(str), length_((str == NULL) ? 0 : Traits::length(str)) { }
  template<typename Alloc>
  BasicStringPiece(const std::basic_string<CharT, Traits, Alloc>& str)  // NOLINT
    : ptr_(str.data()), length_(str.size()) { }
  BasicStringPiece(const CharT* offset, size_type len)
    : ptr_(offset), length_(len) { }
  BasicStringPiece(const BasicStringPiece& str)
    : ptr_(str.ptr_), length_(str.length_) { }

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.
  const CharT* data() const { return ptr_; }
  size_type size() const { return length_; }
  size_type length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }
  void set(const CharT* data, size_type len) {
    ptr_ = data;
    length_ = len;
  }
  void set(const CharT* str) {
    ptr_ = str;
    length_ = str ? Traits::length(str) : 0;
  }
  void set(const void* data, size_type len) {
    ptr_ = reinterpret_cast<const CharT*>(data);
    length_ = len;
  }

  CharT operator[](size_type i) const { return ptr_[i]; }

  void remove_prefix(size_type n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_type n) {
    length_ -= n;
  }

  int compare(const this_type& x) const {
    int r = wordmemcmp(ptr_, x.ptr_, std::min(length_, x.length_));
    if (r == 0) {
      if (length_ < x.length_) r = -1;
      else if (length_ > x.length_) r = +1;
    }
    return r;
  }

  std::basic_string<CharT, Traits> as_string() const {
    // std::basic_string<CharT> doesn't like to
    // take a NULL pointer even with a 0 size.
    if (!empty()) {
      return std::basic_string<CharT, Traits>(data(), size());
    } else {
      return std::basic_string<CharT, Traits>();
    }
  }

  template<typename Alloc>
  void CopyToString(std::basic_string<CharT, Traits, Alloc>* target) const {
    if (!empty()) {
      target->assign(data(), size());
    } else {
      target->assign(0UL, static_cast<CharT>('\0'));
    }
  }

  template<typename Alloc>
  void AppendToString(std::basic_string<CharT, Traits, Alloc>* target) const {
    if (!empty())
      target->append(data(), size());
  }

  // Does "this" start with "x"
  bool starts_with(const this_type& x) const {
    return ((length_ >= x.length_) &&
            (wordmemcmp(ptr_, x.ptr_, x.length_) == 0));
  }

  // Does "this" end with "x"
  bool ends_with(const this_type& x) const {
    return ((length_ >= x.length_) &&
            (wordmemcmp(ptr_ + (length_-x.length_), x.ptr_, x.length_) == 0));
  }

  // standard STL container boilerplate
  typedef CharT value_type;
  typedef const CharT* pointer;
  typedef const CharT& reference;
  typedef const CharT& const_reference;
  typedef ptrdiff_t difference_type;
  static const size_type npos = -1;
  typedef const CharT* const_iterator;
  typedef const CharT* iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  iterator begin() const { return ptr_; }
  iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }

  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  size_type copy(CharT* buf, size_type n, size_type pos = 0) const {
    size_type ret = std::min(length_ - pos, n);
    memcpy(buf, ptr_ + pos, ret);
    return ret;
  }

  size_type find(const this_type& s, size_type pos = 0) const {
    if (pos > length_)
      return npos;

    const CharT * result = std::search(ptr_ + pos, ptr_ + length_,
                                     s.ptr_, s.ptr_ + s.length_);
    const size_type xpos = result - ptr_;
    return xpos + s.length_ <= length_ ? xpos : npos;
  }

  size_type find(CharT c, size_type pos = 0) const {
    if (pos >= length_)
      return npos;

    const CharT * result = std::find(ptr_ + pos, ptr_ + length_, c);
    return result != ptr_ + length_ ? static_cast<size_t>(result - ptr_) : npos;
  }

  size_type rfind(const this_type& s, size_type pos = npos) const {
    if (length_ < s.length_)
      return npos;

    if (s.empty())
      return std::min(length_, pos);

    const CharT * last = ptr_ + std::min(length_ - s.length_, pos) + s.length_;
    const CharT * result = std::find_end(ptr_, last,
                                         s.ptr_, s.ptr_ + s.length_);
    return result != last ? static_cast<size_t>(result - ptr_) : npos;
  }

  size_type rfind(CharT c, size_type pos = npos) const {
    if (length_ == 0)
      return npos;

    for (size_type i = std::min(pos, length_ - 1); ; --i) {
      if (ptr_[i] == c)
        return i;
      if (i == 0)
        break;
    }
    return npos;
  }

  size_type find_first_of(const this_type& s,
                          size_type pos) const {
    if (length_ == 0 || s.length_ == 0)
      return npos;

    // Avoid the cost of BuildLookupTable() for a single-character search.
    if (s.length_ == 1)
      return find_first_of(s.ptr_[0], pos);

    bool lookup[std::numeric_limits<CharT>::max() + 1] = { false };
    BuildLookupTable(s, lookup);
    for (size_type i = pos; i < length_; ++i) {
      if (lookup[static_cast<CharT>(ptr_[i])]) {
        return i;
      }
    }
    return npos;
  }


  size_type find_first_of(CharT c, size_type pos = 0) const {
    return find(c, pos);
  }

  size_type find_first_not_of(const this_type& s,
                              size_type pos) const {
    if (length_ == 0)
      return npos;

    if (s.length_ == 0)
      return 0;

    // Avoid the cost of BuildLookupTable() for a single-character search.
    if (s.length_ == 1)
      return find_first_not_of(s.ptr_[0], pos);

    bool lookup[std::numeric_limits<CharT>::max() + 1] = { false };
    BuildLookupTable(s, lookup);
    for (size_type i = pos; i < length_; ++i) {
      if (!lookup[static_cast<CharT>(ptr_[i])]) {
        return i;
      }
    }
    return npos;
  }

  size_type find_first_not_of(CharT c, size_type pos) const {
    if (length_ == 0)
      return npos;

    for (; pos < length_; ++pos) {
      if (ptr_[pos] != c) {
        return pos;
      }
    }
    return npos;
  }

  size_type find_last_of(const this_type& s, size_type pos) const {
    if (length_ == 0 || s.length_ == 0)
      return npos;

    // Avoid the cost of BuildLookupTable() for a single-character search.
    if (s.length_ == 1)
      return find_last_of(s.ptr_[0], pos);

    bool lookup[std::numeric_limits<CharT>::max() + 1] = { false };
    BuildLookupTable(s, lookup);
    for (size_type i = std::min(pos, length_ - 1); ; --i) {
      if (lookup[static_cast<CharT>(ptr_[i])])
        return i;
      if (i == 0)
        break;
    }
    return npos;
  }

  size_type find_last_of(CharT c, size_type pos = npos) const {
    return rfind(c, pos);
  }

  size_type find_last_not_of(const this_type& s,
                             size_type pos) const {
    if (length_ == 0)
      return npos;

    size_type i = std::min(pos, length_ - 1);
    if (s.length_ == 0)
      return i;

    // Avoid the cost of BuildLookupTable() for a single-character search.
    if (s.length_ == 1)
      return find_last_not_of(s.ptr_[0], pos);

    bool lookup[std::numeric_limits<CharT>::max() + 1] = { false };
    BuildLookupTable(s, lookup);
    for (; ; --i) {
      if (!lookup[static_cast<CharT>(ptr_[i])])
        return i;
      if (i == 0)
        break;
    }
    return npos;
  }

  size_type find_last_not_of(CharT c, size_type pos) const {
    if (length_ == 0)
      return npos;

    for (size_type i = std::min(pos, length_ - 1); ; --i) {
      if (ptr_[i] != c)
        return i;
      if (i == 0)
        break;
    }
    return npos;
  }

  this_type substr(size_type pos, size_type n) const {
    if (pos > length_) pos = length_;
    if (n > length_ - pos) n = length_ - pos;
    return this_type(ptr_ + pos, n);
  }

  static int wordmemcmp(const CharT * p, const CharT * p2, size_type N) {
    return memcmp(p, p2, N);
  }

  static inline void BuildLookupTable(const this_type& characters_wanted,
                                      bool* table) {
    const size_type length = characters_wanted.length();
    const CharT * const data = characters_wanted.data();
    for (size_type i = 0; i < length; ++i) {
      table[static_cast<CharT>(data[i])] = true;
    }
  }
};

template<class CharT>
bool operator==(const BasicStringPiece<CharT>& x,
                const BasicStringPiece<CharT>& y) {
  if (x.size() != y.size())
    return false;

  return BasicStringPiece<CharT>::wordmemcmp(x.data(), y.data(), x.size()) == 0;
}

template<class CharT>
inline bool operator!=(const BasicStringPiece<CharT>& x,
                       const BasicStringPiece<CharT>& y) {
  return !(x == y);
}

template<class CharT>
inline bool operator<(const BasicStringPiece<CharT>& x,
                      const BasicStringPiece<CharT>& y) {
  const int r = BasicStringPiece<CharT>::wordmemcmp(x.data(), y.data(),
                                               std::min(x.size(), y.size()));
  return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

template<class CharT>
inline bool operator>(const BasicStringPiece<CharT>& x,
                      const BasicStringPiece<CharT>& y) {
  return y < x;
}

template<class CharT>
inline bool operator<=(const BasicStringPiece<CharT>& x,
                       const BasicStringPiece<CharT>& y) {
  return !(x > y);
}

template<class CharT>
inline bool operator>=(const BasicStringPiece<CharT>& x,
                       const BasicStringPiece<CharT>& y) {
  return !(x < y);
}

// allow StringPiece to be logged (needed for unit testing).
template<class CharT>
std::ostream& operator<<(std::ostream& o,
                         const BasicStringPiece<CharT>& piece) {
  o.write(piece.data(), static_cast<std::streamsize>(piece.size()));
  return o;
}

typedef BasicStringPiece<char, std::char_traits<char> > StringPiece;

} }  // namespace iv::core
#endif  // _IV_STRINGPIECE_H_
