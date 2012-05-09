#ifndef IV_FIXED_STRING_H_
#define IV_FIXED_STRING_H_
#include <iv/stringpiece.h>
namespace iv {
namespace core {

template<typename CharT, std::size_t MAX>
class BasicFixedString {
 public:
  typedef BasicFixedString<CharT, MAX> this_type;
  typedef CharT char_type;
  typedef std::char_traits<char_type> traits_type;
  typedef typename traits_type::char_type value_type;
  typedef const char_type* pointer;
  typedef const char_type* const_pointer;
  typedef const char_type& reference;
  typedef const char_type& const_reference;
  typedef typename std::size_t size_type;
  typedef typename std::ptrdiff_t difference_type;
  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  static const size_type npos;

  BasicFixedString(const BasicStringPiece<char_type>& str)  // NOLINT
    : size_(str.size()),
      data_() {
    assert(size() <= MAX);
    std::copy(str.begin(), str.end(), data());
    data()[size()] = '\0';
  }

  operator BasicStringPiece<char_type>() {
    return BasicStringPiece<char_type>(data(), size());
  }

  size_type size() const { return size_; }

  bool empty() const { return size() == 0; }

  void clear() {
    *data_ = '\0';
    size_ = 0;
  }

  pointer data() { return data_; }

  const_pointer data() const { return data_; }

  reference operator[](size_type i) {
    assert(i < size());
    return data()[i];
  }

  const_reference operator[](size_type i) const {
    assert(i < size());
    return data()[i];
  }

  reference front() { return data()[0]; }

  const_reference front() const { return data()[0]; }

  reference back() { return data()[size() - 1]; }

  const_reference back() const { return data()[size() - 1]; }

  iterator begin() { return data(); }

  const_iterator begin() const { return data(); }

  const_iterator cbegin() const { return data(); }

  iterator end() { return data() + size(); }

  const_iterator end() const { return data() + size(); }

  const_iterator cend() const { return end(); }

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
    return MAX;
  }

  size_type capacity() const {
    return MAX;
  }

  template<typename Iter>
  void assign(Iter first, Iter last) {
    size_ = std::distance(first, last);
    assert(size() <= MAX);
    std::fill(first, last, data());
    data()[size()] = '\0';
  }

  void assign(size_type n, const_reference x = char_type()) {
    size_ = n;
    assert(size() <= MAX);
    std::fill_n(data(), n, x);
    data()[size()] = '\0';
  }

  void push_back(const_reference value) {
    assert((size() + 1) <= MAX);
    data()[size()] = value;
    size_ += 1;
    data()[size()] = '\0';
  }

  void append(const BasicStringPiece<char_type>& str) {
    assert((size() + str.size()) <= MAX);
    std::copy(str.begin(), str.end(), begin() + size());
    size_ = size() + str.size();
    data()[size()] = '\0';
  }

  void pop_back() {
    assert(size() > 0);
    size_ -= 1;
    data()[size()] = '\0';
  }

 private:
  size_type size_;
  char_type data_[MAX + 1];
};

} }  // namespace iv::core
#endif  // IV_FIXED_STRING_H_
