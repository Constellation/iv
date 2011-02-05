#ifndef _IV_LV5_JSSTRING_H_
#define _IV_LV5_JSSTRING_H_
#include <algorithm>
#include <iterator>
#include <cassert>
#include <gc/gc_cpp.h>
#include "conversions.h"
#include "noncopyable.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "gc_template.h"
#include "ustring.h"
#include "icu/uconv.h"
#include "icu/ustream.h"

namespace iv {
namespace lv5 {

class Context;
class JSStringBuilder;
class JSString : public gc_cleanup {
 public:
  friend class JSStringBuilder;
  typedef JSString this_type;
  typedef GCUString value_type;
  typedef value_type::iterator iterator;
  typedef value_type::const_iterator const_iterator;
  typedef value_type::reverse_iterator reverse_iterator;
  typedef value_type::const_reverse_iterator const_reverse_iterator;
  typedef value_type::size_type size_type;

  JSString()
    : string_(),
      hash_value_(core::StringToHash(string_)) {
  }

  template<class String>
  explicit JSString(const String& rhs)
    : string_(rhs.begin(), rhs.end()),
      hash_value_(core::StringToHash(string_)) {
  }

  JSString(const JSString& str)
    : string_(str.string_),
      hash_value_(str.hash_value_) {
  }

  JSString(size_type len, uc16 ch)
    : string_(len, ch),
      hash_value_(core::StringToHash(string_)) {
  }

  JSString(const uc16* s, size_type len)
    : string_(s, len),
      hash_value_(core::StringToHash(string_)) {
  }

  JSString(const GCUString& s, size_type index, size_type len)
    : string_(s, index, len),
      hash_value_(core::StringToHash(string_)) {
  }

  core::UStringPiece ToPiece() const {
    return core::UStringPiece(data(), size());
  }

  template<typename Iter>
  JSString(Iter start, Iter last)
    : string_(start, last),
      hash_value_(core::StringToHash(string_)) {
  }

  const uc16& operator[](size_type n) const {
    return string_[n];
  }

  bool empty() const {
    return string_.empty();
  }

  size_type size() const {
    return string_.size();
  }

  const uc16* data() const {
    return string_.data();
  }

  const_iterator begin() const {
    return string_.begin();
  }

  const_iterator end() const {
    return string_.end();
  }

  const_reverse_iterator rbegin() const {
    return string_.rbegin();
  }

  const_reverse_iterator rend() const {
    return string_.rend();
  }

  const GCUString& value() const {
    return string_;
  }

  core::UStringPiece piece() const {
    return core::UStringPiece(data(), size());
  }

  inline friend bool operator==(const this_type& lhs,
                                const this_type& rhs) {
    return lhs.string_ == rhs.string_;
  }

  inline friend bool operator<(const this_type& lhs,
                               const this_type& rhs) {
    return lhs.string_ < rhs.string_;
  }

  inline friend bool operator>(const this_type& lhs,
                               const this_type& rhs) {
    return lhs.string_ > rhs.string_;
  }

  inline friend bool operator<=(const this_type& lhs,
                                const this_type& rhs) {
    return lhs.string_ <= rhs.string_;
  }

  inline friend bool operator>=(const this_type& lhs,
                                const this_type& rhs) {
    return lhs.string_ >= rhs.string_;
  }

  inline std::size_t hash_value() const {
    return hash_value_;
  }

  static JSString* New(Context* ctx, const core::StringPiece& str) {
    JSString* const res = new JSString();
    icu::ConvertToUTF16(str, &res->string_);
    res->ReCalcHash();
    return res;
  }

  static JSString* New(Context* ctx, const core::UStringPiece& str) {
    return new JSString(str.data(), str.size());
  }

  static JSString* NewAsciiString(Context* ctx,
                                  const core::StringPiece& str) {
    return new JSString(str.begin(), str.end());
  }

  template<typename Iter>
  static JSString* New(Context* ctx, Iter it, Iter last) {
    return new JSString(it, last);
  }

  static JSString* NewEmptyString(Context* ctx) {
    return new JSString();
  }

 private:
  // ready to create
  explicit JSString(size_type n)
    : string_() {
    string_.resize(n);
  }

  inline void ReCalcHash() {
    hash_value_ = core::StringToHash(string_);
  }

  GCUString string_;
  std::size_t hash_value_;
};

inline std::ostream& operator<<(std::ostream& os, const JSString& str) {
  return os << str.value();
}

class JSStringBuilder : private core::Noncopyable<JSStringBuilder>::type {
 public:
  typedef JSStringBuilder this_type;
  JSStringBuilder(Context* ctx)
    : target_(JSString::NewEmptyString(ctx)) {
  }
  void Append(const core::UStringPiece& piece) {
    piece.AppendToString(&target_->string_);
  }
  void Append(const core::StringPiece& piece) {
    target_->string_.append(piece.begin(), piece.end());
  }
  void Append(const JSString& str) {
    target_->string_.append(str.value());
  }
  void Append(char ch) {
    target_->string_.push_back(ch);
  }
  void Append(uc16 ch) {
    target_->string_.push_back(ch);
  }
  template<typename Iter>
  void Append(Iter it, typename JSString::size_type size) {
    target_->string_.append(it, it + size);
  }
  template<typename Iter>
  void Append(Iter start, Iter last) {
    target_->string_.append(start, last);
  }


  // for assignable object
  void append(const core::UStringPiece& piece) {
    piece.AppendToString(&target_->string_);
  }
  void append(const core::StringPiece& piece) {
    target_->string_.append(piece.begin(), piece.end());
  }
  void append(const JSString& str) {
    target_->string_.append(str.value());
  }

  template<typename Iter>
  this_type& assign(Iter start, Iter end) {
    target_->string_.assign(start, end);
    return *this;
  }

  JSString* Build() {
    target_->ReCalcHash();
    return target_;
  }
 private:
  JSString* target_;
};

} }  // namespace iv::lv5

namespace std {
namespace tr1 {

// template specialization for JSString in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::lv5::JSString>
  : public unary_function<iv::lv5::JSString, std::size_t> {
  std::size_t operator()(const iv::lv5::JSString& x) const {
    return x.hash_value();
  }
};

} }  // namespace std::tr1
#endif  // _IV_LV5_JSSTRING_H_
