// This is limited GC vector.
// And destructor is not called.
// This structure is only used for using direct data pointer in JIT code.
#ifndef IV_LV5_STORGE_H_
#define IV_LV5_STORGE_H_
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <iv/noncopyable.h>
#include <gc/gc.h>
namespace iv {
namespace lv5 {

template<typename T>
class Storage : core::Noncopyable<Storage<T> > {
 public:
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

  Storage(std::size_t n)
    : data_(NULL),
      size_(0),
      capacity_(0) {
    resize(n);
  }

  Storage()
    : data_(NULL),
      size_(0),
      capacity_(0) {
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
  reference operator[](size_type n) { return *(data() + n); }
  const_reference operator[](size_type n) const { return *(data() + n); }
  size_type size() const { return size_; }
  size_type capacity() const { return capacity_; }
  bool empty() const { return size_ == 0; }
  void resize(size_type n, value_type c = value_type()) {
    const size_type previous = size();
    if (capacity() < n) {
      capacity_ = core::NextCapacity(n);
      pointer ptr = new(GC)value_type[capacity()];
      std::copy(begin(), end(), ptr);
      data_ = ptr;
      size_ = n;
    }
    if (previous < size()) {
      std::fill(begin() + previous, begin() + size(), c);
    }
  }
  void push_back(value_type c) {
    if (size() == capacity()) {
      capacity_ = core::NextCapacity(size() + 1);
      pointer ptr = new(GC)value_type[capacity()];
      std::copy(begin(), end(), ptr);
      data_ = ptr;
    }
    *(begin() + size()) = c;
    ++size_;
  }
  void clear() { size_ = 0; }

 private:
  T* data_;
  size_type size_;
  size_type capacity_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_STORGE_H_
