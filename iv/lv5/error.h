#ifndef IV_LV5_ERROR_H_
#define IV_LV5_ERROR_H_
#include <algorithm>
#include <cstdio>
#include <iv/detail/memory.h>
#include <iv/ustring.h>
#include <iv/noncopyable.h>
#include <iv/string_view.h>
#include <iv/lv5/jsval_fwd.h>
namespace iv {
namespace lv5 {

class Context;
class Error {
 public:
  typedef Error this_type;
  typedef void (Error::*bool_type)() const;
  typedef std::vector<core::UString> Stack;

  class Dummy;
  class Standard;

#define IV_LV5_ERROR_LIST(V)\
  V(Normal)\
  V(Eval)\
  V(Range)\
  V(Reference)\
  V(Syntax)\
  V(Type)\
  V(URI)\
  V(User)

  enum Code {
#define IV_SEP(N) N,
    IV_LV5_ERROR_LIST(IV_SEP)
#undef IV_SEP
    NUM_OF_CODE
  };

  static const uint8_t kCodeMask = 7;

  enum Kind {
    TYPE_DUMMY = 0,
    TYPE_STANDARD = 8
  };
  static const uint8_t kKindMask = 8;

  virtual void Report(Code code, const core::string_view& str) {
    set_code(code);
  }

  virtual void Report(Code code, const core::u16string_view& str) {
    set_code(code);
  }

  virtual void Report(JSVal val) {
    set_code(User);
  }

  virtual void Clear() { set_code(Normal); }

  Code code() const { return static_cast<Code>(data_ & kCodeMask); }

  void set_code(Code c) {
    data_ = ((data_ & (~kCodeMask)) | c);
  }

  virtual core::UString detail() const { return core::UString(); }

  virtual JSVal value() const { return JSUndefined; }

  operator bool_type() const {
    return code() != Normal ?
        &Error::this_type_does_not_support_comparisons : 0;
  }

  virtual void set_stack(std::shared_ptr<Stack> stack) { }

  virtual std::shared_ptr<Stack> stack() const { return std::shared_ptr<Stack>(); }

  virtual bool RequireMaterialize(Context* ctx) const { return false; }

  virtual JSVal Detail(Context* ctx) { return JSUndefined; }

  virtual void Dump(Context* ctx, FILE* out) { }

  static const char* CodeString(Code code);

 protected:
  Error(Kind k) : data_() {
    set_code(Normal);
    set_kind(k);
    assert(code() == Normal);
    assert(kind() == k);
  }

  bool IsStandard() const { return data_ & kKindMask; }

  bool IsDummy() const { return !IsStandard(); }

  void set_kind(Kind kind) {
    data_ = ((data_ & (~kKindMask)) | kind);
  }

  Kind kind() const { return static_cast<Kind>(data_ & kKindMask); }

  void this_type_does_not_support_comparisons() const { }

 private:
  uint8_t data_;
};

static const std::array<const char*, Error::NUM_OF_CODE + 1> kErrorStrings = { {
#define IV_SEP(N) #N,
  IV_LV5_ERROR_LIST(IV_SEP)
  "NUM_OF_CODE"
#undef IV_SEP
} };

inline const char* Error::CodeString(Code code) {
  return kErrorStrings[code];
}

class Error::Dummy : public Error {
 public:
  Dummy() : Error(TYPE_DUMMY) { }
};

class Error::Standard : public Error {
 public:
  Standard() : Error(TYPE_STANDARD) { }

  virtual void Report(Code code, const core::string_view& str) {
    Error::Report(code, str);
    detail_.assign(str.begin(), str.end());
  }

  virtual void Report(Code code, const core::u16string_view& str) {
    Error::Report(code, str);
    detail_.assign(str.begin(), str.end());
  }

  virtual void Report(JSVal val) {
    Error::Report(val);
    value_ = val;
  }

  virtual void Clear() {
    Error::Clear();
    stack_.reset();
  }

  virtual core::UString detail() const { return detail_; }

  virtual JSVal value() const { return value_; }

  virtual void set_stack(std::shared_ptr<Stack> stack) {
    stack_ = stack;
  }

  virtual std::shared_ptr<Stack> stack() const { return stack_; }

  // This function is implemented at jserror.h
  virtual bool RequireMaterialize(Context* ctx) const;

  // This function is implemented at jserror.h
  virtual JSVal Detail(Context* ctx);

  // This function is implemented at jserror.h
  virtual void Dump(Context* ctx, FILE* out);

 private:
  JSVal value_;
  core::UString detail_;
  std::shared_ptr<Stack> stack_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ERROR_H_
