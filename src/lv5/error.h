#ifndef _IV_LV5_ERROR_H_
#define _IV_LV5_ERROR_H_
#include <algorithm>
#include "ustring.h"
#include "static_assert.h"
#include "stringpiece.h"
#include "lv5/jsval.h"
namespace iv {
namespace lv5 {

class Context;
class Error {
 public:
  typedef Error this_type;
  typedef void (Error::*bool_type)() const;
  enum Code {
    Normal = 0,
    Native,
    Eval,
    Range,
    Reference,
    Syntax,
    Type,
    URI,
    User
  };

  Error()
    : code_(Normal),
      value_(),
      detail_() {
  }

  Error(const Error& rhs)
    : code_(rhs.code_),
      value_(rhs.value_),
      detail_(rhs.detail_) {
  }

  void Report(Code code, const core::StringPiece& str) {
    code_ = code;
    detail_.assign(str.begin(), str.end());
  }

  void Report(Code code, const core::UStringPiece& str) {
    code_ = code;
    detail_.assign(str.begin(), str.end());
  }

  void Report(const JSVal& val) {
    code_ = User;
    value_ = val;
  }

  void Clear() {
    code_ = Normal;
  }

  Code code() const {
    return code_;
  }

  const core::UString detail() const {
    return detail_;
  }

  const JSVal& value() const {
    return value_;
  }

  operator bool_type() const {
    return code_ != Normal ?
        &Error::this_type_does_not_support_comparisons : 0;
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(code_, rhs.code_);
    swap(value_, rhs.value_);
    swap(detail_, rhs.detail_);
  }

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

  this_type& operator=(const this_type& rhs) {
    if (this != &rhs) {
      this_type(rhs).swap(*this);
    }
    return *this;
  }

 private:
  void this_type_does_not_support_comparisons() const { }

  Code code_;
  JSVal value_;
  core::UString detail_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_ERROR_H_
