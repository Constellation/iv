#ifndef IV_STRINGPIECE_H_
#define IV_STRINGPIECE_H_
#include <climits>
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <iterator>
#include <limits>
namespace iv {
namespace core {
namespace detail {

template<class CharT, class Piece>
struct StringPieceFindOf {
  typedef typename Piece::size_type size_type;

  static size_type FindFirstOf(const Piece& that,
                               const Piece& s, size_type pos) {
    if (pos >= that.size()) {
      return Piece::npos;
    }
    const typename Piece::const_iterator it =
        std::find_first_of(that.begin() + pos, that.end(), s.begin(), s.end());
    return (it == that.end()) ? Piece::npos : std::distance(that.begin(), it);
  }

  template<class Iter>
  class NotFinder {
   public:
    NotFinder(const Iter& begin, const Iter& end)
      : begin_(begin), end_(end) { }

    template<class Val>
    bool operator()(const Val& v) const {
      return std::find(begin_, end_, v) == end_;
    }

   private:
    const Iter begin_;
    const Iter end_;
  };

  static size_type FindFirstNotOf(const Piece& that,
                                  const Piece& s, size_type pos) {
    if (pos >= that.size()) {
      return Piece::npos;
    }
    const typename Piece::const_iterator it =
        std::find_if(
            that.begin() + pos, that.end(),
            NotFinder<typename Piece::const_iterator>(s.begin(), s.end()));
    return (it == that.end()) ? Piece::npos : std::distance(that.begin(), it);
  }

  static size_type FindLastOf(const Piece& that,
                              const Piece& s, size_type pos) {
    if (that.empty()) {
      return Piece::npos;
    }
    const size_type index = std::min(pos, that.size() - 1);
    const typename Piece::const_reverse_iterator last = that.rend();
    const typename Piece::const_reverse_iterator start = that.rbegin();
    const typename Piece::const_reverse_iterator it =
        std::find_first_of(start + ((that.size() - 1) - index),
                           last,
                           s.begin(), s.end());
    return (it == last) ?
        Piece::npos : (that.size() - 1) - std::distance(start, it);
  }

  static size_type FindLastNotOf(const Piece& that,
                                 const Piece& s, size_type pos) {
    if (that.empty()) {
      return Piece::npos;
    }
    const size_type index = std::min(pos, that.size() - 1);
    const typename Piece::const_reverse_iterator last = that.rend();
    const typename Piece::const_reverse_iterator start = that.rbegin();
    const typename Piece::const_reverse_iterator it =
        std::find_if(
            start + ((that.size() - 1) - index), last,
            NotFinder<typename Piece::const_iterator>(s.begin(), s.end()));
    return (it == last) ?
        Piece::npos : (that.size() - 1) - std::distance(start, it);
  }
};

template<class Piece>
struct StringPieceFindOf<char, Piece> {
  typedef typename Piece::size_type size_type;

  static size_type FindFirstOf(const Piece& that,
                               const Piece& s, size_type pos) {
    bool lookup[UCHAR_MAX + 1] = { false };
    BuildLookupTable(s, lookup);
    for (size_type i = pos, len = that.size(); i < len; ++i) {
      if (lookup[static_cast<size_type>(that.data()[i])]) {
        return i;
      }
    }
    return Piece::npos;
  }

  static size_type FindFirstNotOf(const Piece& that,
                                  const Piece& s, size_type pos) {
    bool lookup[UCHAR_MAX + 1] = { false };
    BuildLookupTable(s, lookup);
    for (size_type i = pos, len = that.size(); i < len; ++i) {
      if (!lookup[static_cast<size_type>(that.data()[i])]) {
        return i;
      }
    }
    return Piece::npos;
  }

  static size_type FindLastOf(const Piece& that,
                              const Piece& s, size_type pos) {
    bool lookup[UCHAR_MAX + 1] = { false };
    BuildLookupTable(s, lookup);
    if (that.empty()) {
      return Piece::npos;
    }
    for (size_type i = std::min(pos, that.size() - 1); ; --i) {
      if (lookup[static_cast<size_type>(that.data()[i])]) {
        return i;
      }
      if (i == 0) {
        break;
      }
    }
    return Piece::npos;
  }

  static size_type FindLastNotOf(const Piece& that,
                                 const Piece& s, size_type pos) {
    bool lookup[UCHAR_MAX + 1] = { false };
    BuildLookupTable(s, lookup);
    if (that.empty()) {
      return Piece::npos;
    }
    for (size_type i = std::min(pos, that.size() - 1); ; --i) {
      if (!lookup[static_cast<size_type>(that.data()[i])]) {
        return i;
      }
      if (i == 0) {
        break;
      }
    }
    return Piece::npos;
  }

  static void BuildLookupTable(const Piece& characters_wanted, bool* table) {
    const size_type length = characters_wanted.length();
    const typename Piece::const_pointer data = characters_wanted.data();
    for (size_type i = 0; i < length; ++i) {
      table[static_cast<size_type>(data[i])] = true;
    }
  }
};

}  // namespace detail

template<class CharT, class Traits = std::char_traits<CharT> >
class BasicStringPiece {
 public:
  typedef BasicStringPiece<CharT, Traits> this_type;

  // standard STL container boilerplate
  typedef Traits traits_type;
  typedef typename Traits::char_type value_type;
  typedef const CharT* pointer;
  typedef const CharT* const_pointer;
  typedef const CharT& reference;
  typedef const CharT& const_reference;
  typedef typename std::size_t size_type;
  typedef typename std::ptrdiff_t difference_type;
  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  static const size_type npos;

 public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  BasicStringPiece() : ptr_(NULL), length_(0) { }

  BasicStringPiece(const_pointer str)  // NOLINT
    : ptr_(str), length_((str == NULL) ? 0 : Traits::length(str)) { }

  template<class Alloc>
  BasicStringPiece(const std::basic_string<CharT, Traits, Alloc>& str)  // NOLINT
    : ptr_(str.data()), length_(str.size()) { }

  BasicStringPiece(const_pointer offset, size_type len)
    : ptr_(offset), length_(len) { }

  BasicStringPiece(const BasicStringPiece& str)
    : ptr_(str.ptr_), length_(str.length_) { }

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.
  const_pointer data() const {
    return ptr_;
  }

  size_type size() const {
    return length_;
  }

  size_type length() const {
    return length_;
  }

  bool empty() const {
    return length_ == 0;
  }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }

  void set(const_pointer data, size_type len) {
    ptr_ = data;
    length_ = len;
  }

  void set(const_pointer str) {
    ptr_ = str;
    length_ = str ? Traits::length(str) : 0;
  }

  void set(const void* data, size_type len) {
    ptr_ = reinterpret_cast<const_pointer>(data);
    length_ = len;
  }

  const_reference operator[](size_type i) const {
    return ptr_[i];
  }

  void remove_prefix(size_type n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_type n) {
    length_ -= n;
  }

  int compare(const this_type& x) const {
    const int r = Traits::compare(ptr_, x.ptr_, std::min(length_, x.length_));
    if (r == 0) {
      if (length_ < x.length_) {
        return -1;
      } else if (length_ > x.length_) {
        return 1;
      }
    }
    return r;
  }

  template<class Alloc>
  operator std::basic_string<CharT, Traits, Alloc>() const {
    // std::basic_string<CharT> doesn't like to
    // take a NULL pointer even with a 0 size.
    if (empty()) {
      return std::basic_string<CharT, Traits, Alloc>();
    } else {
      return std::basic_string<CharT, Traits, Alloc>(data(), size());
    }
  }

  template<class Alloc>
  void CopyToString(std::basic_string<CharT, Traits, Alloc>* target) const {
    if (!empty()) {
      target->assign(data(), size());
    } else {
      target->assign(0UL, static_cast<CharT>('\0'));
    }
  }

  template<class Alloc>
  void AppendToString(std::basic_string<CharT, Traits, Alloc>* target) const {
    if (!empty()) {
      target->append(data(), size());
    }
  }

  // Does "this" start with "x"
  bool starts_with(const this_type& x) const {
    return ((length_ >= x.length_) &&
            (Traits::compare(ptr_, x.ptr_, x.length_) == 0));
  }

  // Does "this" end with "x"
  bool ends_with(const this_type& x) const {
    return ((length_ >= x.length_) &&
            (Traits::compare(ptr_ + (length_ - x.length_),
                             x.ptr_,
                             x.length_) == 0));
  }

  const_iterator begin() const {
    return ptr_;
  }

  const_iterator cbegin() const {
    return begin();
  }

  const_iterator end() const {
    return ptr_ + length_;
  }

  const_iterator cend() const {
    return end();
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return rbegin();
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return rend();
  }

  size_type max_size() const {
    return length_;
  }

  size_type capacity() const {
    return length_;
  }

  size_type copy(pointer buf, size_type n, size_type pos = 0) const {
    const size_type ret = std::min(length_ - pos, n);
    Traits::copy(buf, ptr_ + pos, ret);
    return ret;
  }

  size_type find(const this_type& s, size_type pos = 0) const {
    if (pos > length_) {
      return npos;
    }

    const const_pointer result = std::search(ptr_ + pos, ptr_ + length_,
                                             s.ptr_, s.ptr_ + s.length_);
    const size_type xpos = result - ptr_;
    return xpos + s.length_ <= length_ ? xpos : npos;
  }

  size_type find(CharT c, size_type pos = 0) const {
    if (pos >= length_) {
      return npos;
    }
    const const_pointer result = Traits::find(ptr_ + pos, length_ - pos, c);
    return (result) ? static_cast<size_type>(result - ptr_) : npos;
  }

  size_type rfind(const this_type& s, size_type pos = npos) const {
    if (length_ < s.length_) {
      return npos;
    }

    if (s.empty()) {
      return std::min(length_, pos);
    }

    const const_pointer last =
        ptr_ + std::min(length_ - s.length_, pos) + s.length_;
    const const_pointer result = std::find_end(ptr_, last,
                                               s.ptr_, s.ptr_ + s.length_);
    return result != last ? static_cast<size_type>(result - ptr_) : npos;
  }

  size_type rfind(CharT c, size_type pos = npos) const {
    if (empty()) {
      return npos;
    }
    const size_type index = std::min(pos, length_ - 1);
    const const_reverse_iterator last = rend();
    const const_reverse_iterator start = rbegin();
    const const_reverse_iterator it =
        std::find(start + ((length_ - 1) - index), last, c);
    return (it == last) ? npos : ((length_ - 1) - (std::distance(start, it)));
  }

  size_type find_first_of(const this_type& s, size_type pos = 0) const {
    if (empty() || s.empty()) {
      return npos;
    }

    if (s.size() == 1) {
      return find_first_of(s.ptr_[0], pos);
    }

    return detail::StringPieceFindOf<
        CharT,
        this_type>::FindFirstOf(*this, s, pos);
  }

  size_type find_first_of(CharT c, size_type pos = 0) const {
    return find(c, pos);
  }

  size_type find_first_not_of(const this_type& s, size_type pos = 0) const {
    if (empty()) {
      return npos;
    }

    if (s.empty()) {
      return 0;
    }

    if (s.size() == 1) {
      return find_first_not_of(s.ptr_[0], pos);
    }

    return detail::StringPieceFindOf<
        CharT,
        this_type>::FindFirstNotOf(*this, s, pos);
  }

  size_type find_first_not_of(CharT c, size_type pos = 0) const {
    if (empty()) {
      return npos;
    }

    for (; pos < length_; ++pos) {
      if (ptr_[pos] != c) {
        return pos;
      }
    }
    return npos;
  }

  size_type find_last_of(const this_type& s, size_type pos = npos) const {
    if (empty() || s.empty()) {
      return npos;
    }

    if (s.size() == 1) {
      return find_last_of(s.ptr_[0], pos);
    }

    return detail::StringPieceFindOf<
        CharT,
        this_type>::FindLastOf(*this, s, pos);
  }

  size_type find_last_of(CharT c, size_type pos = npos) const {
    return rfind(c, pos);
  }

  size_type find_last_not_of(const this_type& s, size_type pos = npos) const {
    if (empty()) {
      return npos;
    }

    if (empty()) {
      return std::min(pos, length_ - 1);
    }

    if (s.size() == 1) {
      return find_last_not_of(s.ptr_[0], pos);
    }
    return detail::StringPieceFindOf<
        CharT,
        this_type>::FindLastNotOf(*this, s, pos);
  }

 private:
  class NotEqualer {
   public:
    explicit NotEqualer(const CharT& c) : c_(c) { }
    template<class Val>
    bool operator()(const Val& v) const {
      return v != c_;
    }
   private:
    CharT c_;
  };

 public:
  size_type find_last_not_of(CharT c, size_type pos = npos) const {
    if (empty()) {
      return npos;
    }
    const size_type index = std::min(pos, length_ - 1);
    const const_reverse_iterator it =
        std::find_if(rbegin() + ((length_ - 1) - index),
                     rend(),
                     NotEqualer(c));
    return (it == rend()) ?
        npos : ((length_ - 1) - (std::distance(rbegin(), it)));
  }

  this_type substr(size_type pos = 0, size_type n = npos) const {
    if (pos > length_) {
      pos = length_;
    }
    if (n > length_ - pos) {
      n = length_ - pos;
    }
    return this_type(ptr_ + pos, n);
  }

  friend bool operator==(const this_type& x, const this_type& y) {
    return x.compare(y) == 0;
  }

  friend bool operator!=(const this_type& x, const this_type& y) {
    return !(x == y);
  }

  friend bool operator<(const this_type& x, const this_type& y) {
    return x.compare(y) < 0;
  }

  friend bool operator>(const this_type& x, const this_type& y) {
    return x.compare(y) > 0;
  }

  friend bool operator<=(const this_type& x, const this_type& y) {
    return x.compare(y) <= 0;
  }

  friend bool operator>=(const this_type& x, const this_type& y) {
    return x.compare(y) >= 0;
  }

 private:
  const_pointer ptr_;
  size_type length_;
};

// allow StringPiece to be logged (needed for unit testing).
template<class CharT, class Traits>
std::ostream& operator<<(std::ostream& o,
                         const BasicStringPiece<CharT, Traits>& piece) {
  o.write(piece.data(), static_cast<std::streamsize>(piece.size()));
  return o;
}

template<class CharT, class Traits>
const typename BasicStringPiece<CharT, Traits>::size_type
BasicStringPiece<CharT, Traits>::npos =
  static_cast<typename BasicStringPiece<CharT, Traits>::size_type>(-1);

typedef BasicStringPiece<char, std::char_traits<char> > StringPiece;

} }  // namespace iv::core
#endif  // IV_STRINGPIECE_H_
