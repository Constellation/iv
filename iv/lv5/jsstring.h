#ifndef IV_LV5_JSSTRING_H_
#define IV_LV5_JSSTRING_H_
#include <cstdlib>
#include <new>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <gc/gc_cpp.h>
#include <iv/detail/cstdint.h>
#include <iv/debug.h>
#include <iv/string_view.h>
#include <iv/lv5/error.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/jscell.h>
namespace iv {
namespace lv5 {

class Context;
class GlobalData;
class JSArray;
class Error;
class JSVal;

class JSFlatString;
class JSConsString;
class JSSeqString;
class JSSlicedString;
class JSExternalString;

template<typename CharT>
class JSFlatTypedString;

class JSString : public JSCell {
 public:
  friend class GlobalData;

  typedef JSString this_type;
  typedef int32_t size_type;

  static const size_type kMaxSize = core::symbol::kMaxSize;
  static const size_type npos = -1;

  enum {
    IS_8BIT            = 1u,
    MASK_STRING        = 3u << 1u,
    STRING_CONS        = 0u << 1u,
    STRING_SEQ         = 1u << 1u,
    STRING_SLICED      = 2u << 1u,
    STRING_EXTERNAL    = 3u << 1u,
  };

  size_type size() const { return size_; }

  bool empty() const { return size() == 0; }

  bool Is8Bit() const { return flags_ & IS_8BIT; }

  bool Is16Bit() const { return !Is8Bit(); }

  bool IsFlat() const {
    // not cons
    return (flags_ & MASK_STRING) != STRING_CONS;
  }

  template<typename CharT>
  core::basic_string_view<CharT> view() const {
    return core::basic_string_view<CharT>(data<CharT>(), size());
  }

  core::string_view view8() const { return view<char>(); }

  core::u16string_view view16() const { return view<char16_t>(); }

  inline const JSFlatString* Flatten() const;

  const JSFlatTypedString<char>* Flatten8() const;

  const JSFlatTypedString<char16_t>* Flatten16() const;

  uint32_t type() const {
    return (flags_ & MASK_STRING);
  }

  size_type find(const JSString& target, size_type index) const;

  size_type rfind(const JSString& target, size_type index) const;

  JSString* Substring(Context* ctx, size_type from, size_type to = npos) const;

  JSString* Repeat(Context* ctx, uint32_t count, Error* e);

  JSArray* Split(Context* ctx, uint32_t limit, Error* e) const;

  JSArray* Split(Context* ctx, char16_t ch, uint32_t limit, Error* e) const;

  struct Hasher {
    std::size_t operator()(this_type* str) const {
      if (str->Is8Bit()) {
        return core::Hash::StringToHash(str->view8());
      } else {
        return core::Hash::StringToHash(str->view16());
      }
    }
  };

  struct Equaler {
    bool operator()(this_type* lhs, this_type* rhs) const {
      return *lhs == *rhs;
    }
  };

  // TODO(Constellation) implement it
  void MarkChildren(radio::Core* core) { }

  static std::size_t SizeOffset() {
    return IV_OFFSETOF(JSString, size_);
  }

  std::string GetUTF8() const;

  std::u16string GetUTF16() const;

  int compare(const this_type& x) const;

  friend bool operator==(const this_type& x, const this_type& y);

  friend bool operator!=(const this_type& x, const this_type& y);

  friend bool operator<(const this_type& x, const this_type& y);

  friend bool operator>(const this_type& x, const this_type& y);

  friend bool operator<=(const this_type& x, const this_type& y);

  friend bool operator>=(const this_type& x, const this_type& y);

  int compare(const core::string_view& view) const {
    if (Is8Bit()) {
      return view8().compare(view);
    }
    const core::u16string_view v = view16();
    return core::CompareIterators(v.begin(), v.end(), view.begin(), view.end());
  }

  int compare(const core::u16string_view& view) const {
    if (Is16Bit()) {
      return view16().compare(view);
    }
    const core::string_view v = view8();
    return core::CompareIterators(v.begin(), v.end(), view.begin(), view.end());
  }

  char16_t At(size_type n) const;

  template<typename CharT>
  CharT* data() {
    assert((std::is_same<CharT, char>::value) ? Is8Bit() : Is16Bit());
    Flatten();
    return static_cast<CharT*>(data_.ptr);
  }

  template<typename CharT>
  const CharT* data() const {
    assert((std::is_same<CharT, char>::value) ? Is8Bit() : Is16Bit());
    Flatten();
    return static_cast<const CharT*>(data_.ptr);
  }

  char* data8() { return data<char>(); }

  char16_t* data16() { return data<char16_t>(); }

  const char* data8() const { return data<char>(); }

  const char16_t* data16() const { return data<char16_t>(); }

  // factories

  static JSString* New(Context* ctx, Symbol sym);

  static JSString* NewEmpty(Context* ctx);

  static JSString* New(Context* ctx, char16_t ch);

  static JSString* New(Context* ctx, core::string_view view, Error* e);

  static JSString* New(Context* ctx, core::u16string_view view, Error* e);

  template<typename Iter>
  static JSString* New(Context* ctx,
                       Iter it, Iter last,
                       bool is_8bit, Error* e);

  static JSString* NewCons(Context* ctx, JSVal* src, uint32_t count, Error* e);

  static JSString* NewCons(Context* ctx,
                           JSString* lhs, JSString* rhs, Error* e);

  static JSString* NewExternal(Context* ctx, core::string_view view);

  static JSString* NewExternal(Context* ctx, core::u16string_view view);

 protected:
  JSString(Context* ctx, int32_t size);

  JSString(Context* ctx);  // empty

  // We ensure that JSString's length is less than INT32_MAX.
  int32_t size_;
  mutable uint32_t flags_;
  mutable union {
    void*     ptr;
    char*     c8;
    char16_t* c16;
    JSString* str;  // This is used for JSConsString
  } data_;
};

template<typename CharT>
class JSFlatTypedString : public JSString {
 public:
  friend class JSString;

  typedef JSFlatTypedString this_type;
  typedef CharT char_type;
  typedef std::char_traits<char_type> traits_type;

  typedef char_type* iterator;
  typedef const char_type* const_iterator;
  typedef typename std::iterator_traits<iterator>::value_type value_type;
  typedef typename std::iterator_traits<iterator>::pointer pointer;
  typedef typename std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef typename std::iterator_traits<iterator>::reference reference;
  typedef typename std::iterator_traits<const_iterator>::reference const_reference;  // NOLINT
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef typename std::iterator_traits<iterator>::difference_type difference_type;  // NOLINT
  typedef int32_t size_type;

 public:
  operator core::basic_string_view<CharT>() const {
    return view<CharT>();
  }

  const_reference operator[](size_type n) const {
    assert(n < size());
    return (data())[n];
  }

  reference operator[](size_type n) {
    assert(n < size());
    return (data())[n];
  }

  pointer data() {
    return JSString::data<CharT>();
  }

  const_pointer data() const {
    return JSString::data<CharT>();
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

  inline friend bool operator==(const this_type& x, const this_type& y) {
    return x.compare(y) == 0;
  }

  inline friend bool operator!=(const this_type& x, const this_type& y) {
    return !(x == y);
  }

  inline friend bool operator<(const this_type& x, const this_type& y) {
    return x.compare(y) < 0;
  }

  inline friend bool operator>(const this_type& x, const this_type& y) {
    return x.compare(y) > 0;
  }

  inline friend bool operator<=(const this_type& x, const this_type& y) {
    return x.compare(y) <= 0;
  }

  inline friend bool operator>=(const this_type& x, const this_type& y) {
    return x.compare(y) >= 0;
  }
};

typedef JSFlatTypedString<char> JSAsciiFlatString;
typedef JSFlatTypedString<char16_t> JSUTF16FlatString;

class JSFlatString : public JSString {
 public:
  friend class JSString;

  typedef JSFlatString this_type;

 private:
  template<typename T>
  class iterator_base
    : public std::iterator<std::random_access_iterator_tag, char16_t> {
   public:
    typedef iterator_base<T> this_type;
    typedef std::iterator<std::random_access_iterator_tag, T> super_type;
    typedef typename super_type::pointer pointer;
    typedef const typename super_type::pointer const_pointer;
    typedef char16_t reference;
    typedef char16_t const_reference;
    typedef typename super_type::difference_type difference_type;
    typedef iterator_base<typename std::remove_const<T>::type> iterator;
    typedef iterator_base<typename std::add_const<T>::type> const_iterator;

    iterator_base(T fiber)  // NOLINT
      : fiber_(fiber), position_(0) { }

    iterator_base(const iterator& it)  // NOLINT
      : fiber_(it.fiber()), position_(it.position()) { }

    const_reference operator*() const {
      return (*fiber_)[position_];
    }

    const_reference operator[](difference_type d) const {
      return (*fiber_)[position_ + d];
    }

    this_type& operator++() {
      return ((*this) += 1);
    }

    this_type& operator++(int) {  // NOLINT
      this_type iter(*this);
      (*this)++;
      return iter;
    }

    this_type& operator--() {
      return ((*this) -= 1);
    }

    this_type& operator--(int) {  // NOLINT
      this_type iter(*this);
      (*this)--;
      return iter;
    }

    this_type& operator+=(difference_type d) {
      position_ += d;
      return *this;
    }

    this_type& operator-=(difference_type d) {
      return ((*this) += (-d));
    }

    friend bool operator==(const this_type& lhs, const this_type& rhs) {
      return lhs.position() == rhs.position();
    }
    friend bool operator!=(const this_type& lhs, const this_type& rhs) {
      return lhs.position() != rhs.position();
    }
    friend bool operator<(const this_type& lhs, const this_type& rhs) {
      return lhs.position() < rhs.position();
    }
    friend bool operator<=(const this_type& lhs, const this_type& rhs) {
      return lhs.position() <= rhs.position();
    }
    friend bool operator>(const this_type& lhs, const this_type& rhs) {
      return lhs.position() > rhs.position();
    }
    friend bool operator>=(const this_type& lhs, const this_type& rhs) {
      return lhs.position() >= rhs.position();
    }

    friend this_type operator+(const this_type& lhs, difference_type d) {
      this_type iter(lhs);
      return iter += d;
    }
    friend this_type operator-(const this_type& lhs, difference_type d) {
      this_type iter(lhs);
      return iter -= d;
    }
    friend difference_type operator-(const this_type& lhs,
                                     const this_type& rhs) {
      return lhs.position() - rhs.position();
    }

    T fiber() const { return fiber_; }

    difference_type position() const { return position_; }

   private:
    T fiber_;
    difference_type position_;
  };

 public:
  typedef iterator_base<const this_type*> const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  const_iterator begin() const { return const_iterator(this); }

  const_iterator cbegin() const { return begin(); }

  const_iterator end() const { return begin() + size(); }

  const_iterator cend() const { return end(); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const { return rbegin(); }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const { return rend(); }

 public:
  char16_t operator[](size_type n) const;

  int compare(const this_type& x) const;

  template<typename CharT>
  inline const JSFlatTypedString<CharT>* As() const {
    return static_cast<const JSFlatTypedString<CharT>*>(
        static_cast<const JSString*>(this));
  }

  template<typename CharT>
  inline JSFlatTypedString<CharT>* As() {
    return static_cast<JSFlatTypedString<CharT>*>(static_cast<JSString*>(this));
  }

  inline friend bool operator==(const this_type& x, const this_type& y) {
    return x.compare(y) == 0;
  }

  inline friend bool operator!=(const this_type& x, const this_type& y) {
    return !(x == y);
  }

  inline friend bool operator<(const this_type& x, const this_type& y) {
    return x.compare(y) < 0;
  }

  inline friend bool operator>(const this_type& x, const this_type& y) {
    return x.compare(y) > 0;
  }

  inline friend bool operator<=(const this_type& x, const this_type& y) {
    return x.compare(y) <= 0;
  }

  inline friend bool operator>=(const this_type& x, const this_type& y) {
    return x.compare(y) >= 0;
  }
};

class JSSeqString : public JSString {
 public:
  friend class JSConsString;
  friend class JSString;
  friend class GlobalData;

 private:
  template<typename CharT>
  static JSSeqString* NewUninitialized(Context* ctx, int32_t len) {
    void* mem = Allocate<CharT>(ctx, len);
    return new (mem) JSSeqString(ctx, std::is_same<CharT, char>::value, len);
  }

  template<typename Iter>
  JSSeqString(Context* ctx, bool is_8bit, Iter it, Iter last)
    : JSString(ctx, (last - it)) {
    flags_ |= STRING_SEQ;
    if (is_8bit) {
      flags_ |= IS_8BIT;
      data_.c8 = GetPayload<char>();
      std::copy(it, last, data_.c8);
    } else {
      data_.c16 = GetPayload<char16_t>();
      std::copy(it, last, data_.c16);
    }
  }

  JSSeqString(Context* ctx, bool is_8bit, int32_t len)
    : JSString(ctx, len) {
    flags_ = STRING_SEQ;
    if (is_8bit) {
      flags_ |= IS_8BIT;
      data_.c8 = GetPayload<char>();
    } else {
      data_.c16 = GetPayload<char16_t>();
    }
  }

  template<typename CharT>
  static std::size_t GetControlSize() {
    return IV_ROUNDUP(sizeof(JSSeqString), sizeof(CharT));
  }

  template<typename CharT>
  CharT* GetPayload() {
    return reinterpret_cast<CharT*>(this) +
        GetControlSize<CharT>() / sizeof(CharT);
  }

  template<typename CharT>
  static void* Allocate(Context* ctx, std::size_t length) {
    // TODO(Yusuke Suzuki):
    // Move allocation to ctx
    const std::size_t size = GetControlSize<CharT>() + (sizeof(CharT) * length);
    return GC_MALLOC_ATOMIC(size);
  }
};

class JSSlicedString : public JSString {
 private:
  friend class JSString;
  friend class JSConsString;

  void ConvertFromCons(JSSeqString* buf, int32_t offset) const {
    assert(size() != 0 && size() != 1);
    flags_ = STRING_SLICED;
    if (buf->Is8Bit()) {
      flags_ |= IS_8BIT;
      data_.c8 = buf->data8() + offset;
    } else {
      data_.c16 = buf->data16() + offset;
    }
    original_ = buf;
  }

  static JSString* New(Context* ctx, JSString* str, int32_t from, int32_t to) {
    assert(to - from >= 0);
    const int32_t size = to - from;
    if (size == 0) {
      return NewEmpty(ctx);
    }
    if (size == 1) {
      return JSString::New(ctx, str->At(from));
    }
    return new JSSlicedString(ctx, str, from, to);
  }

  JSSlicedString(Context* ctx, JSString* str, int32_t from, int32_t to)
    : JSString(ctx, to - from)
    , original_(str) {
    flags_ = STRING_SLICED;
    str->Flatten();
    if (str->Is8Bit()) {
      data_.c8 = str->data8() + from;
      flags_ |= IS_8BIT;
    } else {
      data_.c16 = str->data16() + from;
    }
  }

  mutable JSString* original_;  // refer to the original
};

class JSConsString : public JSString {
 public:
  friend class JSString;

  JSString* Flatten() const {
    if (Is8Bit()) {
      JSSeqString* buf = JSSeqString::NewUninitialized<char>(ctx(), size());
      Enumerate(
          this,
          0,
          [buf](const JSString* flat, int32_t offset) {
            // All flat strings are 8bit.
            const core::string_view view = flat->view8();
            std::copy(view.begin(), view.end(), buf->data8() + offset);
          },
          [buf](const JSConsString* cons, int32_t offset) {
            static_cast<const JSSlicedString*>(
                static_cast<const JSString*>(cons))->ConvertFromCons(buf,
                                                                     offset);
          });
      return buf;
    } else {
      JSSeqString* buf = JSSeqString::NewUninitialized<char16_t>(ctx(), size());
      Enumerate(
          this,
          0,
          [buf](const JSString* flat, int32_t offset) {
            if (flat->Is8Bit()) {
              const core::string_view view = flat->view8();
              std::copy(view.begin(), view.end(), buf->data16() + offset);
            } else {
              const core::u16string_view view = flat->view16();
              std::copy(view.begin(), view.end(), buf->data16() + offset);
            }
          },
          [buf](const JSConsString* cons, int32_t offset) {
            static_cast<const JSSlicedString*>(
                static_cast<const JSString*>(cons))->ConvertFromCons(buf,
                                                                     offset);
          });
      return buf;
    }
  }

 private:
  template<typename Func1, typename Func2>
  static void Enumerate(const JSString* current,
                        int32_t offset,
                        const Func1& do_flat,
                        const Func2& do_cons) {
    do {
      if (current->IsFlat()) {
        do_flat(current, offset);
        return;
      }

      const JSConsString* cons = static_cast<const JSConsString*>(current);
      const JSString* lhs = cons->lhs();
      const JSString* rhs = cons->rhs();
      const int32_t slice_offset = offset;
      // Recursively flatten shorter string.
      // This ensures that depth of recursion is <= log N (N is string length).
      if (lhs->size() > rhs->size()) {
        Enumerate(rhs, offset + lhs->size(), do_flat, do_cons);
        current = lhs;
      } else {
        Enumerate(lhs, offset, do_flat, do_cons);
        current = rhs;
        offset += lhs->size();
      }
      do_cons(cons, slice_offset);
    } while (true);
  }

  JSConsString(Context* ctx,
               JSString* lhs,
               JSString* rhs)
    : JSString(ctx, lhs->size() + rhs->size())
    , ctx_(ctx) {
    flags_ = STRING_CONS;
    data_.str = lhs;
    rhs_ = rhs;
    if (lhs->Is8Bit() && rhs->Is8Bit()) {
      flags_ = IS_8BIT;
    }
  }

  JSString* lhs() const { return data_.str; }
  JSString* rhs() const { return rhs_; }
  Context* ctx() const { return ctx_; }

  JSString* rhs_;
  Context* ctx_;  // FIXME(Yusuke SuzukI): This is due to Flatten operation.
};

static_assert(sizeof(JSSlicedString) <= sizeof(JSConsString),
              "JSConsString is larger than or equal to JSSlicedString.");

class JSExternalString : public JSString {
 public:
  friend class JSString;

 private:
  // TODO(Yusuke Suzuki):
  // Remove these const_cast.
  JSExternalString(Context* ctx, core::string_view view)
    : JSString(ctx, view.size()) {
    flags_ = STRING_EXTERNAL | IS_8BIT;
    data_.c8 = const_cast<char*>(view.data());
  }

  JSExternalString(Context* ctx, core::u16string_view view)
    : JSString(ctx, view.size()) {
    flags_ = STRING_EXTERNAL;
    data_.c16 = const_cast<char16_t*>(view.data());
  }
};

std::ostream& operator<<(std::ostream& os, const JSString& str);

inline const JSFlatString* JSString::Flatten() const {
  if (!IsFlat()) {
    // We need to flatten JSConsString.
    static_cast<const JSConsString*>(this)->Flatten();
  }
  return static_cast<const JSFlatString*>(this);
}

inline const JSFlatTypedString<char>* JSString::Flatten8() const {
  assert(Is8Bit());
  Flatten();
  return static_cast<const JSAsciiFlatString*>(this);
}

inline const JSFlatTypedString<char16_t>* JSString::Flatten16() const {
  assert(Is16Bit());
  Flatten();
  return static_cast<const JSUTF16FlatString*>(this);
}

inline int JSString::compare(const JSString& x) const {
  return Flatten()->compare(*x.Flatten());
}

inline bool operator==(const JSString& x, const JSString& y) {
  if (x.size() == y.size()) {
    return x.compare(y) == 0;
  }
  return false;
}

inline bool operator!=(const JSString& x, const JSString& y) {
  return !(x == y);
}

inline bool operator<(const JSString& x, const JSString& y) {
  return (*x.Flatten()) < (*y.Flatten());
}

inline bool operator>(const JSString& x, const JSString& y) {
  return (*x.Flatten()) > (*y.Flatten());
}

inline bool operator<=(const JSString& x, const JSString& y) {
  return (*x.Flatten()) <= (*y.Flatten());
}

inline bool operator>=(const JSString& x, const JSString& y) {
  return (*x.Flatten()) >= (*y.Flatten());
}

template<typename Iter>
inline JSString* JSString::New(Context* ctx,
                               Iter it, Iter last, bool is_8bit, Error* e) {
  const int32_t size = std::distance(it, last);
  if (size == 0) {
    return NewEmpty(ctx);
  }
  if (size == 1) {
    return New(ctx, *it);
  }
  if (is_8bit) {
    void* mem = JSSeqString::Allocate<char>(ctx, size);
    return new (mem) JSSeqString(ctx, true, it, last);
  } else {
    void* mem = JSSeqString::Allocate<char16_t>(ctx, size);
    return new (mem) JSSeqString(ctx, false, it, last);
  }
}

inline JSString* JSString::New(Context* ctx,
                               core::string_view view,
                               Error* e) {
  return New(ctx, view.begin(), view.end(), true, e);
}

inline JSString* JSString::New(Context* ctx,
                               core::u16string_view view,
                               Error* e) {
  return New(ctx, view.begin(), view.end(), false, e);
}

inline JSString* JSString::NewExternal(Context* ctx, core::string_view view) {
  if (view.empty()) {
    return NewEmpty(ctx);
  }
  if (view.size() == 1) {
    return New(ctx, view[0]);
  }
  return new JSExternalString(ctx, view);
}

inline JSString* JSString::NewExternal(Context* ctx,
                                       core::u16string_view view) {
  if (view.empty()) {
    return NewEmpty(ctx);
  }
  if (view.size() == 1) {
    return New(ctx, view[0]);
  }
  return new JSExternalString(ctx, view);
}

inline JSString* JSString::NewCons(Context* ctx,
                                   JSString* lhs, JSString* rhs, Error* e) {
  if (lhs->empty()) {
    return rhs;
  } else if (rhs->empty()) {
    return lhs;
  }
  const int64_t new_size = static_cast<int64_t>(lhs->size()) + rhs->size();
  if (new_size > kMaxSize) {
    e->Report(Error::Type, "too long string is prohibited");
    return nullptr;
  }
  return new JSConsString(ctx, lhs, rhs);
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_H_
