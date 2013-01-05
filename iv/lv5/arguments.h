#ifndef IV_LV5_ARGUMENTS_H_
#define IV_LV5_ARGUMENTS_H_
#include <vector>
#include <iterator>
#include <algorithm>
#include <iv/noncopyable.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsval_fwd.h>

namespace iv {
namespace lv5 {

class Context;

class Arguments : private core::Noncopyable<> {
 public:
  typedef Arguments this_type;

  typedef JSVal* reverse_iterator;
  typedef const JSVal* const_reverse_iterator;
  // arguments order is reversed
  typedef std::reverse_iterator<reverse_iterator> iterator;
  typedef std::reverse_iterator<const_reverse_iterator> const_iterator;

  typedef std::iterator_traits<iterator>::value_type value_type;

  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;

  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  inline iterator begin() { return iterator(rend()); }

  inline const_iterator begin() const { return const_iterator(crend()); }

  inline const_iterator cbegin() const { return begin(); }

  inline iterator end() { return iterator(rbegin()); }

  inline const_iterator end() const { return const_iterator(rbegin()); }

  inline const_iterator cend() const { return end(); }

  inline reverse_iterator rbegin() {
    return stack_ - size_;
  }

  inline const_reverse_iterator rbegin() const {
    return stack_ - size_;
  }

  inline const_reverse_iterator crbegin() const {
    return stack_ - size_;
  }

  inline reverse_iterator rend() {
    return stack_;
  }

  inline const_reverse_iterator rend() const {
    return stack_;
  }

  inline const_reverse_iterator crend() const {
    return stack_;
  }

  size_type max_size() const { return size_; }

  size_type capacity() const { return size_; }

  size_type size() const { return size_; }

  bool empty() const { return size_ == 0; }

  reference operator[](size_type n) {
    assert(size() > n);
    return *(stack_ - (n + 1));
  }

  const_reference operator[](size_type n) const {
    assert(size() > n);
    return *(stack_ - (n + 1));
  }

  reference front() {
    assert(!empty());
    return (*this)[0];
  }

  const_reference front() const {
    assert(!empty());
    return (*this)[0];
  }

  reference back() {
    assert(!empty());
    return (*this)[size_ - 1];
  }

  const_reference back() const {
    assert(!empty());
    return (*this)[size_ - 1];
  }

  JSVal At(size_type n) const {
    if (n < size_) {
      return (*this)[n];
    } else {
      return JSUndefined;
    }
  }

  bool IsDefined(size_type n) const {
    return n < size_;
  }

  Context* ctx() const { return ctx_; }

  inline JSVal this_binding() const {
    return stack_[0];
  }

  inline void set_this_binding(JSVal binding) {
    stack_[0] = binding;
  }

  inline void set_constructor_call(bool val) {
    constructor_call_ = val;
  }

  inline bool IsConstructorCalled() const {
    return constructor_call_;
  }

  pointer ExtractBase() const {
    return stack_ - size();
  }

 protected:
  // Arguments Layout
  // [arg3][arg2][arg1][this]
  //                     ^
  //                     stack_

  // for ScopedArguments
  Arguments(Context* ctx, std::size_t n, Error* e)
    : ctx_(ctx),
      stack_(),
      size_(n),
      constructor_call_(false) { }

  // for VM
  Arguments(Context* ctx, pointer ptr, std::size_t n)
    : ctx_(ctx),
      stack_(ptr),
      size_(n),
      constructor_call_(false) { }

  Context* ctx_;
  pointer stack_;
  size_type size_;
  bool constructor_call_;
};

// Implementation is defined in context.h
class ScopedArguments : public Arguments {
 public:
  // layout is
  // [arg2][arg1][this]
  ScopedArguments(Context* ctx, std::size_t n, Error* e);
  ~ScopedArguments();
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ARGUMENTS_H_
