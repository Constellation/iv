#ifndef _IV_LV5_JSSTRING_H_
#define _IV_LV5_JSSTRING_H_
#include <algorithm>
#include <iterator>
#include <cassert>
#include <gc/gc_cpp.h>
#include "conversions.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "gc-template.h"

namespace iv {
namespace lv5 {

class Context;
// TODO(Constellation) protected inheritance of GCUString
class JSString : public GCUString, public gc {
 public:
//  using GCUString::iterator;
//  using GCUString::get_allocator;
//  using GCUString::max_size;
//  using GCUString::size_type;
//  using GCUString::size;
//  using GCUString::length;
//  using GCUString::empty;
//  using GCUString::capacity;
//  using GCUString::assign;
//  using GCUString::begin;
//  using GCUString::end;
//  using GCUString::rbegin;
//  using GCUString::rend;
//  using GCUString::operator[];
//  using GCUString::at;
//  using GCUString::resize;
//  using GCUString::push_back;
//  using GCUString::insert;
//  using GCUString::erase;
//  using GCUString::clear;
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
//  using GCUString::operator==;
//  using GCUString::operator!=;
//  using GCUString::operator<;
//  using GCUString::operator>;
//  using GCUString::operator<=;
//  using GCUString::operator>=;
//  using GCUString::operator+;
//  using GCUString::operator+=;
//  using GCUString::operator<<;
//  using GCUString::operator>>;
//  using GCUString::pop_back;
//  using GCUString::getline;
//  using GCUString::reverse;
//  using GCUString::front;
//  using GCUString::back;

  JSString();
  template<class String>
  explicit JSString(const String& rhs)
    : GCUString(rhs.begin(), rhs.end()),
      hash_value_(core::StringToHash(*this)) {
  }

  JSString(const JSString& str);
  JSString(size_type len, uc16 ch);
  explicit JSString(const uc16* s);  // NOLINT
  JSString(const uc16* s, size_type len);
  JSString(const GCUString& s, size_type index, size_type len);
  template<typename Iter>
  JSString(Iter start, Iter last)
    : GCUString(start, last),
      hash_value_(core::StringToHash(*this)) {
  }

  inline std::size_t hash_value() const {
    return hash_value_;
  }

  static JSString* New(Context* context, const core::StringPiece& str);
  static JSString* New(Context* context, const core::UStringPiece& str);
  static JSString* NewAsciiString(Context* context,
                                  const core::StringPiece& str);
  static JSString* NewEmptyString(Context* ctx);
 protected:
  std::size_t hash_value_;
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
