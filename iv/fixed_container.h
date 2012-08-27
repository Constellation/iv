#ifndef IV_FIXED_CONTAINER_H_
#define IV_FIXED_CONTAINER_H_
#include <cstddef>
#include <cassert>
#include <iterator>
#include <iv/detail/type_traits.h>
namespace iv {
namespace core {

// FixedContainer
// not have ownership to buffer.
// this have external buffer and size and provide interface such as std::array
template<typename T>
class FixedContainer {
 public:
  typedef T value_type;
  typedef typename std::add_pointer<value_type>::type pointer;
  typedef typename std::add_pointer<const value_type>::type const_pointer;
  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef T&  reference;
  typedef const T& const_reference;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  FixedContainer(pointer buffer, size_type size)
    : buf_(buffer),
      size_(size) { }

  iterator begin() {
    return buf_;
  }

  iterator end() {
    return buf_ + size_;
  }

  const_iterator begin() const {
    return buf_;
  }

  const_iterator cbegin() const {
    return buf_;
  }

  const_iterator end() const {
    return buf_ + size_;
  }

  const_iterator cend() const {
    return buf_ + size_;
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }

  reference operator[](size_type n) {
    assert(n < size_);
    return buf_[n];
  }

  const_reference operator[](size_type n) const {
    assert(n < size_);
    return buf_[n];
  }

  reference front() {
    return buf_[0];
  }

  const_reference front() const {
    return buf_[0];
  }

  reference back() {
    return buf_[size_ - 1];
  }

  const_reference back() const {
    return buf_[size_ - 1];
  }

  size_type size() const {
    return size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  size_type max_size() const {
    return size_;
  }

  pointer data() {
    return buf_;
  }

  const_pointer data() const {
    return buf_;
  }

  void assign(const T& value) {
    std::fill_n(begin(), size(), value);
  }

  void swap(FixedContainer<T>& rhs) {
    using std::swap;
    swap(buf_, rhs.buf_);
    swap(size_, rhs.size_);
  }

  friend inline void swap(FixedContainer<T>& lhs, FixedContainer<T> rhs) {
    lhs.swap(rhs);
  }

 private:
  pointer buf_;
  size_type size_;
};

} }  // namespace iv::core
#endif  // IV_FIXED_CONTAINER_H_
