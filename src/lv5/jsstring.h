#ifndef _IV_LV5_JSSTRING_H_
#define _IV_LV5_JSSTRING_H_
#include <algorithm>
#include <iterator>
#include <cassert>
#include <vector>
#include <functional>
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include "unicode.h"
#include "conversions.h"
#include "noncopyable.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "ustring.h"
#include "static_assert.h"
#include "lv5/gc_template.h"
#include "lv5/heap_object.h"
namespace iv {
namespace lv5 {

class Context;

static const std::size_t kMaxFibers = 5;

class StringFiber : private core::Noncopyable<StringFiber> {
 public:
  typedef StringFiber this_type;
  typedef uint16_t char_type;
  typedef std::char_traits<char_type> traits_type;

  typedef char_type* iterator;
  typedef const char_type* const_iterator;
  typedef std::iterator_traits<iterator>::value_type value_type;
  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  template<typename String>
  static this_type* New(const String& piece) {
    this_type* mem = static_cast<this_type*>(GC_selective_alloc(
        sizeof(size_type) + piece.size() * sizeof(char_type),
        GC_true_type()));
    mem->size_ = piece.size();
    std::copy(piece.begin(), piece.end(), mem->begin());
    return mem;
  }

  static this_type* NewWithSize(std::size_t n) {
    this_type* mem = static_cast<this_type*>(GC_selective_alloc(
        sizeof(size_type) + n * sizeof(char_type),
        GC_true_type()));
    mem->size_ = n;
    return mem;
  }

  template<typename Iter>
  static this_type* New(Iter it, Iter last) {
    const std::size_t n = std::distance(it, last);
    this_type* mem = static_cast<this_type*>(GC_selective_alloc(
        sizeof(size_type) + n * sizeof(char_type),
        GC_true_type()));
    mem->size_ = n;
    std::copy(it, last, mem->begin());
    return mem;
  }

  template<typename Iter>
  static this_type* New(Iter it, std::size_t n) {
    this_type* mem = static_cast<this_type*>(GC_selective_alloc(
        sizeof(size_type) + n * sizeof(char_type),
        GC_true_type()));
    mem->size_ = n;
    std::copy(it, it + n, mem->begin());
    return mem;
  }

  operator core::UStringPiece() const {
    return core::UStringPiece(data(), size());
  }

  const_reference operator[](size_type n) const {
    return (data())[n];
  }

  reference operator[](size_type n) {
    return (data())[n];
  }

  pointer data() {
    return reinterpret_cast<pointer>(this + 1);
  }

  const_pointer data() const {
    return reinterpret_cast<const_pointer>(this + 1);
  }

  iterator begin() {
    return data();
  }

  const_iterator begin() const {
    return data();
  }

  const_iterator cbegin() const {
    return data();
  }

  iterator end() {
    return begin() + size_;
  }

  const_iterator end() const {
    return begin() + size_;
  }

  const_iterator cend() const {
    return begin() + size_;
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return rbegin();
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return rend();
  }

  size_type size() const {
    return size_;
  }

  int compare(const this_type& x) const {
    const int r =
        traits_type::compare(data(), x.data(), std::min(size_, x.size_));
    if (r == 0) {
      if (size_ < x.size_) {
        return -1;
      } else if (size_ > x.size_) {
        return 1;
      }
    }
    return r;
  }

  friend bool operator==(const this_type& x, const this_type& y) {
    return x.compare(y) == 0;
  }

  friend bool operator!=(const this_type& x, const this_type& y) {
    return !(x == y);
  }

  friend bool operator<(const this_type& x, const this_type& y) {
    return x.compare(y) < 0;
  }

  friend bool operator>(const this_type& x, const this_type& y) {
    return x.compare(y) > 0;
  }

  friend bool operator<=(const this_type& x, const this_type& y) {
    return x.compare(y) <= 0;
  }

  friend bool operator>=(const this_type& x, const this_type& y) {
    return x.compare(y) >= 0;
  }

 private:
  StringFiber();  // hiding constructor

  std::size_t size_;
};

IV_STATIC_ASSERT(sizeof(StringFiber) == sizeof(std::size_t));

class JSString : public HeapObject {
 public:
  typedef JSString this_type;
  typedef StringFiber Fiber;
  typedef std::array<const Fiber*, kMaxFibers> Fibers;
  typedef Fiber::size_type size_type;

  struct FlattenTag { };

  std::size_t size() const {
    return size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  const Fiber* Flatten() const {
    if (fiber_count_ != 1) {
      Fiber* fiber = Fiber::NewWithSize(size_);
      Fiber::iterator target = fiber->begin();
      for (Fibers::iterator it = fibers_.begin(),
           last = it + fiber_count_; it != last; ++it) {
        target = std::copy((*it)->begin(), (*it)->end(), target);
        *it = NULL;
      }
      fiber_count_ = 1;
      fibers_[0] = fiber;
    }
    assert(fibers_[0]->size() == size());
    return fibers_[0];
  }

  std::string GetUTF8() const {
    const Fiber* fiber = Flatten();
    std::string str;
    str.reserve(size());
    if (core::unicode::UTF16ToUTF8(
          fiber->begin(), fiber->end(),
          std::back_inserter(str)) != core::unicode::NO_ERROR) {
      str.clear();
    }
    return str;
  }

  core::UString GetUString() const {
    const Fiber* fiber = Flatten();
    return core::UString(fiber->data(), fiber->size());
  }

  uint16_t GetAt(size_type n) const {
    if (fibers_[0]->size() > n) {
      return (*fibers_.front())[n];
    }
    return (*Flatten())[n];
  }

  template<typename Target>
  void CopyToString(Target* target) const {
    if (!empty()) {
      const Fiber* fiber = Flatten();
      target->assign(fiber->data(), fiber->size());
    } else {
      target->assign(0UL, typename Target::value_type());
    }
  }

  template<typename Target>
  void AppendToString(Target* target) const {
    if (!empty()) {
      const Fiber* fiber = Flatten();
      target->assign(fiber->data(), fiber->size());
    }
  }

  template<typename OutputIter>
  void Copy(OutputIter target) const {
    for (Fibers::iterator it = fibers_.begin(),
         last = it + fiber_count_; it != last; ++it) {
      target = std::copy((*it)->begin(), (*it)->end(), target);
    }
  }

  friend bool operator==(const this_type& x, const this_type& y) {
    if (x.size() == y.size()) {
      return (*x.Flatten()) == (*y.Flatten());
    }
    return false;
  }

  friend bool operator!=(const this_type& x, const this_type& y) {
    return !(x == y);
  }

  friend bool operator<(const this_type& x, const this_type& y) {
    return (*x.Flatten()) < (*y.Flatten());
  }

  friend bool operator>(const this_type& x, const this_type& y) {
    return (*x.Flatten()) > (*y.Flatten());
  }

  friend bool operator<=(const this_type& x, const this_type& y) {
    return (*x.Flatten()) <= (*y.Flatten());
  }

  friend bool operator>=(const this_type& x, const this_type& y) {
    return (*x.Flatten()) >= (*y.Flatten());
  }

  static this_type* New(Context* ctx, const core::StringPiece& str) {
    std::vector<uint16_t> buffer;
    buffer.reserve(str.size());
    if (core::unicode::UTF8ToUTF16(
            str.begin(),
            str.end(),
            std::back_inserter(buffer)) != core::unicode::NO_ERROR) {
      buffer.clear();
    }
    return new this_type(
        core::UStringPiece(buffer.data(), buffer.size()));
  }

  static this_type* New(Context* ctx, const core::UStringPiece& str) {
    return new this_type(str);
  }

  static this_type* NewAsciiString(Context* ctx,
                                   const core::StringPiece& str) {
    return new this_type(str);
  }

  static this_type* NewSingle(Context* ctx, uint16_t ch) {
    return new this_type(ch);
  }

  template<typename Iter>
  static this_type* New(Context* ctx, Iter it, Iter last) {
    return new this_type(it, last);
  }

  static this_type* NewEmptyString(Context* ctx) {
    return new this_type();
  }

  static this_type* New(Context* ctx, this_type* lhs, this_type* rhs) {
    if (lhs->empty()) {
      return rhs;
    } else if (rhs->empty()) {
      return lhs;
    } else if ((lhs->fiber_count_ + rhs->fiber_count_) <= kMaxFibers) {
      return new this_type(lhs, rhs);
    } else {
      // flatten version
      return new this_type(lhs, rhs, FlattenTag());
    }
  }

  static this_type* New(Context* ctx, const Fiber* fiber) {
    return new this_type(fiber);
  }

 private:
  std::size_t fiber_count() const {
    return fiber_count_;
  }

  // empty string
  JSString()
    : size_(0),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Fiber::NewWithSize(size_);
  }

  // single char string
  JSString(uint16_t ch)
    : size_(1),
      fiber_count_(1),
      fibers_() {
    Fiber* fiber = Fiber::NewWithSize(1);
    (*fiber)[0] = ch;
    fibers_[0] = fiber;
  }

  template<typename Iter>
  JSString(Iter it, Iter last)
    : size_(std::distance(it, last)),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Fiber::New(it, size_);
  }

  template<typename String>
  explicit JSString(const String& str)
    : size_(str.size()),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Fiber::New(str.data(), size_);
  }

  // fiber count version
  JSString(JSString* lhs, JSString* rhs)
    : size_(lhs->size() + rhs->size()),
      fiber_count_(lhs->fiber_count_ + rhs->fiber_count_),
      fibers_() {
    assert(fiber_count_ <= kMaxFibers);
    std::copy(
        rhs->fibers_.begin(),
        rhs->fibers_.begin() + rhs->fiber_count_,
        std::copy(lhs->fibers_.begin(),
                  lhs->fibers_.begin() + lhs->fiber_count_,
                  fibers_.begin()));
  }

  // flatten version
  JSString(JSString* lhs, JSString* rhs, FlattenTag tag)
    : size_(lhs->size() + rhs->size()),
      fiber_count_(1),
      fibers_() {
    Fiber* fiber = Fiber::NewWithSize(size_);
    Fiber::iterator target = fiber->begin();
    for (Fibers::const_iterator it = lhs->fibers_.begin(),
         last = lhs->fibers_.begin() + lhs->fiber_count_;
         it != last; ++it) {
      target = std::copy((*it)->begin(), (*it)->end(), target);
    }
    for (Fibers::const_iterator it = rhs->fibers_.begin(),
         last = rhs->fibers_.begin() + rhs->fiber_count_;
         it != last; ++it) {
      target = std::copy((*it)->begin(), (*it)->end(), target);
    }
    fibers_[0] = fiber;
  }

  explicit JSString(const Fiber* fiber)
    : size_(fiber->size()),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = fiber;
  }

  std::size_t size_;
  mutable std::size_t fiber_count_;
  mutable Fibers fibers_;
};

inline std::ostream& operator<<(std::ostream& os, const JSString& str) {
  return os << str.GetUTF8();
}

class StringBuilder : protected std::vector<uint16_t> {
 public:
  typedef StringBuilder this_type;
  typedef std::vector<uint16_t> container_type;

  void Append(const core::UStringPiece& piece) {
    insert(end(), piece.begin(), piece.end());
  }

  void Append(const core::StringPiece& piece) {
    insert(end(), piece.begin(), piece.end());
  }

  void Append(const JSString& str) {
    str.Copy(std::back_inserter<container_type>(*this));
  }

  void Append(uint16_t ch) {
    push_back(ch);
  }

  template<typename Iter>
  void Append(Iter it, typename container_type::size_type size) {
    insert(end(), it, it + size);
  }

  template<typename Iter>
  void Append(Iter start, Iter last) {
    insert(end(), start, last);
  }

  // for assignable object (like std::string)

  void append(const core::UStringPiece& piece) {
    insert(end(), piece.begin(), piece.end());
  }

  void append(const core::StringPiece& piece) {
    insert(end(), piece.begin(), piece.end());
  }

  void append(const JSString& str) {
    str.Copy(std::back_inserter<container_type>(*this));
  }

  using container_type::push_back;

  using container_type::insert;

  using container_type::assign;

  JSString* Build(Context* ctx) const {
    return JSString::New(ctx, begin(), end());
  }

  core::UStringPiece BuildUStringPiece() const {
    return core::UStringPiece(data(), size());
  }

  core::UString BuildUString() const {
    return core::UString(data(), size());
  }
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSSTRING_H_
