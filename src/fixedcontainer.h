#ifndef IV_FIXEDCONTAINE_H_
#define IV_FIXEDCONTAINE_H_
#include <cstddef>
#include <iterator>
#include <tr1/type_traits>
namespace iv {
namespace core {

template<typename T>
class FixedContainer {
 public:
  typedef T value_type;
  typedef typename std::tr1::add_pointer<T>::type pointer;
  typedef pointer iterator;
  typedef typename std::tr1::add_const<iterator>::type const_iterator;
  typedef typename std::tr1::add_reference<T>::type  reference;
  typedef typename std::tr1::add_const<reference>::type const_reference;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::ptrdiff_t difference_type;
  typedef std::size_t size_type;
  FixedContainer(pointer buffer, size_type size)
    : buf_(buffer),
      size_(size) { }
  // iterator begin() { return buf_; }
  // iterator end() { return buf_+size_; }
  const_iterator begin() const { return buf_; }
  const_iterator end() const { return buf_+size_; }
  size_type size() const { return size_; }
  reference operator[](std::size_t n) {
    return *(buf_+n);
  }
  const_reference operator[](std::size_t n) const {
    return *(buf_+n);
  }
 protected:
  pointer buf_;
  size_type size_;
};

} }  // namespace iv::core
#endif  // IV_FIXEDCONTAINER_H_
