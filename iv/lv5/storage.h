// This is limited GC vector.
// And destructor is not called.
// This structure is only used for using direct data pointer in JIT code.
#ifndef IV_LV5_STORGE_H_
#define IV_LV5_STORGE_H_
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <gc/gc.h>
#include <iv/noncopyable.h>
#include <iv/utils.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {

template<typename T>
class Storage {
 public:
  friend class breaker::Compiler;
  friend class breaker::MonoIC;

  typedef std::size_t size_type;
  typedef T value_type;
  typedef T* iterator;
  typedef const T* const_iterator;
  typedef T& reference;
  typedef const T& const_reference;
  typedef std::ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef Storage<T> this_type;

  Storage(pointer ptr, size_type n, size_type cap)
    : data_(ptr),
      size_(n),
      capacity_(cap) {
  }

  Storage(size_type n, value_type v = value_type())
    : data_(NULL),
      size_(0),
      capacity_(0) {
    resize(n, v);
  }

  Storage()
    : data_(NULL),
      size_(0),
      capacity_(0) {
  }

  Storage(const this_type& v)
    : data_(NULL),
      size_(0),
      capacity_(0) {
    assign(v.begin(), v.end());
  }

  this_type& operator=(const this_type& v) {
    assign(v.begin(), v.end());
    return *this;
  }

  pointer data() { return data_; }
  const_pointer data() const { return data_; }
  iterator begin() { return data(); }
  const_iterator begin() const { return data(); }
  const_iterator cbegin() const { return data(); }
  iterator end() { return data() + size(); }
  const_iterator end() const { return data() + size(); }
  const_iterator cend() const { return data() + size(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }
  reference operator[](size_type n) {
    assert(size() > n);
    return *(data() + n);
  }
  const_reference operator[](size_type n) const {
    assert(size() > n);
    return *(data() + n);
  }
  size_type size() const { return size_; }
  size_type capacity() const { return capacity_; }
  bool empty() const { return size_ == 0; }
  void resize(size_type n, value_type c = value_type()) {
    const size_type previous = size();
    reserve(n);
    size_ = n;
    if (previous < size()) {
      std::fill(begin() + previous, begin() + size(), c);
    }
  }
  void push_back(value_type c) {
    reserve(size() + 1);
    *(begin() + size()) = c;
    ++size_;
  }
  void pop_back() {
    --size_;
  }
  void clear() { size_ = 0; }
  void reserve(size_type n) {
    if (n > capacity()) {
      capacity_ = core::NextCapacity(n);
      pointer ptr = new(GC)value_type[capacity()];
      std::copy(begin(), end(), ptr);
      data_ = ptr;
    }
  }
  size_type max_size() const { return std::numeric_limits<size_type>::max(); }
  reference front() { return (*this)[0]; }
  const_reference front() const { return (*this)[0]; }
  reference back() { return (*this)[size() - 1]; }
  const_reference back() const { return (*this)[size() - 1]; }
  iterator insert(iterator position, const JSVal& x) {
    const difference_type offset = position - begin();
    reserve(size() + 1);
    const iterator it(begin() + offset);
    const iterator last(end());
    size_ += 1;
    std::copy_backward(it, last, end());
    (*it) = x;
    return it;
  }
  void insert(iterator position, size_type n, const JSVal& x) {
    const difference_type offset = position - begin();
    reserve(size() + n);
    const iterator it(begin() + offset);
    const iterator last(end());
    size_ += n;
    std::copy_backward(it, last, end());
    std::fill(it, it + n, x);
  }
  template<typename InputIterator>
  void insert(iterator position, InputIterator first, InputIterator last) {
    const difference_type offset = position - begin();
    const size_type n = std::distance(first, last);
    reserve(size() + n);
    const iterator it(begin() + offset);
    const iterator olast(end());
    size_ += n;
    std::copy_backward(it, olast, end());
    std::copy(first, last, it);
  }
  template<typename InputIterator>
  void assign(InputIterator first, InputIterator last) {
    clear();
    const size_type n = std::distance(first, last);
    reserve(n);
    size_ = n;
    std::copy(first, last, begin());
  }
  void assign(size_type n, const JSVal& u) {
    clear();
    reserve(n);
    size_ = n;
    std::fill(begin(), end(), u);
  }
  iterator erase(iterator position) {
    const iterator it(position + 1);
    std::copy(it, end(), position);
    --size_;
    return it;
  }
  iterator erase(iterator first, iterator last) {
    const size_type n = std::distance(first, last);
    std::copy(last, end(), first);
    size_ -= n;
    return last;
  }

  static std::size_t DataOffset() {
    return IV_OFFSETOF(this_type, data_);
  }

  static std::size_t SizeOffset() {
    return IV_OFFSETOF(this_type, size_);
  }

  static std::size_t CapacityOffset() {
    return IV_OFFSETOF(this_type, capacity_);
  }

 private:
  T* data_;  // This is GC target pointer
  size_type size_;
  size_type capacity_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_STORGE_H_
