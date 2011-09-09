#ifndef IV_LV5_ARGUMENTS_H_
#define IV_LV5_ARGUMENTS_H_
#include <vector>
#include <iterator>
#include <algorithm>
#include "noncopyable.h"
#include "lv5/error.h"
#include "lv5/jsval_fwd.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {

class Context;

class Arguments : private core::Noncopyable<> {
 public:
  typedef Arguments this_type;

  typedef JSVal* iterator;
  typedef const JSVal* const_iterator;

  typedef std::iterator_traits<iterator>::value_type value_type;

  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  inline iterator begin() {
    return stack_ + 1;  // index 0 is this binding
  }

  inline const_iterator begin() const {
    return stack_ + 1;
  }

  inline const_iterator cbegin() const {
    return stack_ + 1;
  }

  inline iterator end() {
    return begin() + size_;
  }

  inline const_iterator end() const {
    return begin() + size_;
  }

  inline const_iterator cend() const {
    return begin() + size_;
  }

  inline reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  inline const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  inline const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }

  inline reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  inline const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  inline const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  size_type max_size() const {
    return size_;
  }

  size_type capacity() const {
    return size_;
  }

  size_type size() const {
    return size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  reference operator[](size_type n) {
    return stack_[n + 1];
  }

  const_reference operator[](size_type n) const {
    return stack_[n + 1];
  }

  reference front() {
    return (*this)[0];
  }

  const_reference front() const {
    return (*this)[0];
  }

  reference back() {
    return (*this)[size_ - 1];  // no check
  }

  const_reference back() const {
    return (*this)[size_ - 1];  // no check
  }

  JSVal At(size_type n) const {
    if (n < size_) {
      return stack_[n + 1];
    } else {
      return JSUndefined;
    }
  }

  Context* ctx() const {
    return ctx_;
  }

  inline JSVal this_binding() const {
    return stack_[0];
  }

  inline void set_this_binding(const JSVal& binding) {
    stack_[0] = binding;
  }

  inline void set_constructor_call(bool val) {
    constructor_call_ = val;
  }

  inline bool IsConstructorCalled() const {
    return constructor_call_;
  }

  pointer GetEnd() const {
    return stack_ + 1 + size_;
  }

 protected:
  // for ScopedArguments
  Arguments(Context* ctx, std::size_t n, Error* e)
    : ctx_(ctx),
      stack_(),
      size_(n),
      constructor_call_(false) { }

  // for VM
  Arguments(Context* ctx,
            pointer ptr,
            std::size_t n)
    : ctx_(ctx),
      stack_(ptr),
      size_(n),
      constructor_call_(false) { }

  Context* ctx_;
  pointer stack_;
  size_type size_;
  bool constructor_call_;
};

class ScopedArguments : public Arguments {
 public:
  ScopedArguments(Context* ctx, std::size_t n, Error* e)
    : Arguments(ctx, n, e) {
    stack_ = context::StackGain(ctx_, size_ + 1);
    if (!stack_) {  // stack overflow
      e->Report(Error::Range, "maximum call stack size exceeded");
    } else {
      assert(stack_);
      // init
      std::fill(stack_, stack_ + size_ + 1, JSUndefined);
    }
  }

  ~ScopedArguments() {
    if (stack_) {
      context::StackRelease(ctx_, size_ + 1);
    }
    stack_ = NULL;
  }
};

class VMArguments : public Arguments {
 public:
  VMArguments(Context* ctx,
              pointer ptr,
              std::size_t n)
    : Arguments(ctx, ptr, n) {
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ARGUMENTS_H_
