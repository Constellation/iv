#ifndef IV_LV5_ARGUMENTS_H_
#define IV_LV5_ARGUMENTS_H_
#include <vector>
#include <iterator>
#include <algorithm>
#include "jsval.h"
#include "noncopyable.h"

namespace iv {
namespace lv5 {
class Interpreter;
class Context;

class Arguments : private core::Noncopyable<Arguments>::type {
 public:
  typedef std::vector<JSVal> JSVals;
  typedef Arguments this_type;

  typedef JSVals::value_type value_type;
  typedef JSVals::reference reference;
  typedef JSVals::const_reference const_reference;
  typedef JSVals::iterator iterator;
  typedef JSVals::const_iterator const_iterator;
  typedef JSVals::pointer pointer;
  typedef JSVals::difference_type difference_type;
  typedef JSVals::size_type size_type;

  typedef JSVals::const_reverse_iterator const_reverse_iterator;
  typedef JSVals::reverse_iterator reverse_iterator;

  inline iterator begin() { return args_.begin(); }
  inline const_iterator begin() const { return args_.begin(); }
  inline iterator end() { return args_.end(); }
  inline const_iterator end() const { return args_.end(); }
  inline reverse_iterator rbegin() { return args_.rbegin(); }
  inline const_reverse_iterator rbegin() const { return args_.rbegin(); }
  inline reverse_iterator rend() { return args_.rend(); }
  inline const_reverse_iterator rend() const { return args_.rend(); }

  size_type max_size() const { return args_.max_size(); }
  size_type capacity() const { return args_.capacity(); }
  size_type size() const { return args_.size(); }
  bool empty() const { return args_.empty(); }
  void swap(JSVals& rhs) { args_.swap(rhs); }
  void swap(this_type& rhs) {
    using std::swap;
    args_.swap(rhs.args_);
    swap(ctx_, rhs.ctx_);
    swap(this_binding_, rhs.this_binding_);
  }
  reference operator[](size_type n) { return args_[n]; }
  const_reference operator[](size_type n) const { return args_[n]; }

  explicit Arguments(Context* ctx)
    : ctx_(ctx),
      this_binding_(),
      args_(),
      constructor_call_(false) {
  }

  Arguments(Context* ctx, const JSVal& this_binding)
    : ctx_(ctx),
      this_binding_(this_binding),
      args_(),
      constructor_call_(false) {
  }

  Arguments(Context* ctx, std::size_t n)
    : ctx_(ctx),
      this_binding_(),
      args_(n),
      constructor_call_(false) {
  }

  inline const JSVals& args() const {
    return args_;
  }

  inline void push_back(const JSVal& val) {
    args_.push_back(val);
  }

  Context* ctx() const {
    return ctx_;
  }

  inline JSVal this_binding() const {
    return this_binding_;
  }

  inline void set_this_binding(const JSVal& binding) {
    this_binding_ = binding;
  }

  inline void set_constructor_call(bool val) {
    constructor_call_ = val;
  }

  inline bool IsConstructorCalled() const {
    return constructor_call_;
  }

 private:
  Context* ctx_;
  JSVal this_binding_;
  std::vector<JSVal> args_;
  bool constructor_call_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ARGUMENTS_H_
