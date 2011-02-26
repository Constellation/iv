#ifndef IV_LV5_ARGUMENTS_H_
#define IV_LV5_ARGUMENTS_H_
#include <vector>
#include <iterator>
#include <algorithm>
#include "error.h"
#include "noncopyable.h"
#include "lv5/jsval.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {
class Interpreter;
class Context;

class Arguments : private core::Noncopyable<Arguments>::type {
 public:
  typedef Arguments this_type;

  typedef VMStack::value_type value_type;
  typedef VMStack::reference reference;
  typedef VMStack::const_reference const_reference;
  typedef VMStack::iterator iterator;
  typedef VMStack::const_iterator const_iterator;
  typedef VMStack::pointer pointer;
  typedef VMStack::difference_type difference_type;
  typedef VMStack::size_type size_type;

  typedef VMStack::const_reverse_iterator const_reverse_iterator;
  typedef VMStack::reverse_iterator reverse_iterator;

  inline iterator begin() {
    return stack_ + 1;  // index 0 is this binding
  }

  inline const_iterator begin() const {
    return stack_ + 1;
  }

  inline iterator end() {
    return begin() + size_;
  }

  inline const_iterator end() const {
    return begin() + size_;
  }

  inline reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  inline const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  inline reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  inline const_reverse_iterator rend() const {
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

  explicit Arguments(Context* ctx, Error* e)
    : ctx_(ctx),
      stack_(),
      size_(0),
      constructor_call_(false) {
    stack_ = context::stack(ctx_)->Gain(size_ + 1);
    if (!stack_) {  // stack overflow
      e->Report(Error::Range, "maximum call stack size exceeded");
    } else {
      assert(stack_);
      // init
      std::fill(stack_, stack_ + size_ + 1, JSUndefined);
    }
  }

  Arguments(Context* ctx, std::size_t n, Error* e)
    : ctx_(ctx),
      stack_(),
      size_(n),
      constructor_call_(false) {
    stack_ = context::stack(ctx_)->Gain(size_ + 1);
    if (!stack_) {  // stack overflow
      e->Report(Error::Range, "maximum call stack size exceeded");
    } else {
      assert(stack_);
      // init
      std::fill(stack_, stack_ + size_ + 1, JSUndefined);
    }
  }

  ~Arguments() {
    if (stack_) {
      context::stack(ctx_)->Release(size_ + 1);
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

 private:
  Context* ctx_;
  VMStack::pointer stack_;
  VMStack::size_type size_;
  bool constructor_call_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ARGUMENTS_H_
