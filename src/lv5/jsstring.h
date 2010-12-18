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
class JSString : public gc {
 public:
  friend class JSStringBuilder;
  typedef JSString this_type;
  typedef GCUString value_type;
  typedef value_type::iterator iterator;
  typedef value_type::const_iterator const_iterator;
  typedef value_type::size_type size_type;

//  using GCUString::capacity;
//  using GCUString::assign;
//  using GCUString::begin;
//  using GCUString::end;
//  using GCUString::rbegin;
//  using GCUString::rend;
//  using GCUString::at;
//  using GCUString::resize;
//  using GCUString::push_back;
//  using GCUString::insert;
//  using GCUString::erase;
//  using GCUString::swap;
//  using GCUString::compare;
//  using GCUString::append;
//  using GCUString::c_str;
//  using GCUString::copy;
//  using GCUString::data;
//  using GCUString::find;
//  using GCUString::rfind;
//  using GCUString::substr;
//  using GCUString::find_first_of;
//  using GCUString::find_first_not_of;
//  using GCUString::find_last_of;
//  using GCUString::find_last_not_of;
//  using GCUString::replace;
//  using GCUString::operator=;
//  using GCUString::operator!=;
//  using GCUString::operator<<;
//  using GCUString::operator>>;
//  using GCUString::pop_back;
//  using GCUString::getline;
//  using GCUString::reverse;
//  using GCUString::front;
//  using GCUString::back;

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

  void clear() {
    string_.clear();
  }

  iterator begin() {
    return string_.begin();
  }

  const_iterator begin() const {
    return string_.begin();
  }

  iterator end() {
    return string_.end();
  }

  const_iterator end() const {
    return string_.end();
  }

  const GCUString& value() const {
    return string_;
  }

  template<typename Iter>
  this_type& assign(Iter start, Iter end) {
    string_.assign(start, end);
    return *this;
  }

  this_type& assign(const this_type& str) {
    string_.assign(str.string_);
    return *this;
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

  inline void ReCalcHash() {
    hash_value_ = core::StringToHash(string_);
  }

  static JSString* New(Context* ctx, const core::StringPiece& str) {
    JSString* const res = new JSString();
    icu::ConvertToUTF16(str, res);
    res->ReCalcHash();
    return res;
  }

  static JSString* New(Context* ctx, const core::UStringPiece& str) {
    return new JSString(str.data(), str.size());
  }

  // TODO(Constellation) more clean static member function
  static JSString* New(Context* ctx,
                       const core::UStringPiece& lhs,
                       const core::UStringPiece& rhs) {
    using std::copy;
    JSString* const res = new JSString(lhs.size() + rhs.size());
    copy(rhs.begin(), rhs.end(),
         copy(lhs.begin(), lhs.end(), res->begin()));
    res->ReCalcHash();
    return res;
  }

  static JSString* NewAsciiString(Context* ctx,
                                  const core::StringPiece& str) {
    return new JSString(str.begin(), str.end());
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

  GCUString string_;
  std::size_t hash_value_;
};

inline std::ostream& operator<<(std::ostream& os, const JSString& str) {
  return os << str.value();
}

class JSStringBuilder : private core::Noncopyable<JSStringBuilder>::type {
 public:
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
