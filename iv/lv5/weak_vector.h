#ifndef IV_LV5_WEAK_VECTOR_H_
#define IV_LV5_WEAK_VECTOR_H_
#include <iv/detail/type_traits.h>
#include <iv/static_assert.h>
#include <iv/noncopyable.h>
#include <gc/gc_cpp.h>
namespace iv {
namespace lv5 {

template<typename T>
class WeakVector : public gc_cleanup, private core::Noncopyable<WeakVector<T> > {
 public:
  IV_STATIC_ASSERT(std::is_pointer<T>::value);
  typedef std::size_t size_type;
  typedef T value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::ptrdiff_t difference_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef WeakVector<T> this_type;

  WeakVector(size_type n)
    : data_(NULL),
      size_(n) {
    data_ = static_cast<value_type*>(GC_MALLOC_ATOMIC(sizeof(value_type) * size()));
  }

  ~WeakVector() {
    for (pointer it = data(), last = data() + size(); it != last; ++it) {
      if (*it) {
        GC_unregister_disappearing_link(reinterpret_cast<void**>(it));
      }
    }
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

  // read only
  value_type operator[](size_type n) const {
    assert(size() > n);
    return *(data() + n);
  }

  void set(size_type n, value_type ptr) {
    assert(size() > n);
    pointer target = data() + n;
    // release
    if (*target) {
      GC_unregister_disappearing_link(reinterpret_cast<void**>(target));
    }
    // register
    *target = ptr;
    if (ptr) {
      GC_general_register_disappearing_link(reinterpret_cast<void**>(target), reinterpret_cast<void*>(ptr));
    }
  }

  size_type size() const { return size_; }
  bool empty() const { return size_ == 0; }
  size_type max_size() const { return std::numeric_limits<size_type>::max(); }
  reference front() { return (*this)[0]; }
  const_reference front() const { return (*this)[0]; }
  reference back() { return (*this)[size() - 1]; }
  const_reference back() const { return (*this)[size() - 1]; }
 private:
  value_type* data_;
  size_type size_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_WEAK_VECTOR_H_
