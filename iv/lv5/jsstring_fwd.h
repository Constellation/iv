#ifndef IV_LV5_JSSTRING_FWD_H_
#define IV_LV5_JSSTRING_FWD_H_
#include <cstdlib>
#include <new>
#include <vector>
#include <iterator>
#include <algorithm>
#include <gc/gc_cpp.h>
#include <iv/detail/cstdint.h>
#include <iv/debug.h>
#include <iv/unicode.h>
#include <iv/ustring.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/lv5/fiber.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/radio/cell.h>
namespace iv {
namespace lv5 {

class Context;
class GlobalData;
class JSArray;
class Error;

// JSString structure is following
//
// + JSString
//     have FiberSlots = {....} <Cons|Fiber>*
//
// + FiberSlot
//   + Fiber
//       have string content
//       this class is published to world
//   + Cons
//       have Fiber array <Cons|Fiber>*
//       this class is only seen in JSString

class JSString : public JSCell {
 public:
  friend class GlobalData;
  typedef JSString this_type;

  // FiberSlots has FiberSlot by reverse order
  // for example, string "THIS" and "IS" to
  // [ "IS", "THIS", NULL, NULL, NULL ]
  typedef Fiber16::size_type size_type;

  static const size_type kMaxFibers = 5;
  static const size_type npos = static_cast<size_type>(-1);
  static const size_type kMaxSize = core::symbol::kMaxSize;

  typedef std::array<const FiberSlot*, kMaxFibers> FiberSlots;

  struct Hasher {
    std::size_t operator()(this_type* str) const {
      if (str->Is8Bit()) {
        return core::Hash::StringToHash(*str->Get8Bit());
      } else {
        return core::Hash::StringToHash(*str->Get16Bit());
      }
    }
  };

  struct Equaler {
    bool operator()(this_type* lhs, this_type* rhs) const {
      return *lhs == *rhs;
    }
  };

 private:
  template<typename Derived>
  class FiberEnumerator {
   public:
    template<typename Iter>
    FiberEnumerator(Iter it, Iter last) : slots_(it, last) { }

    void Consume() {
      while (true) {
        const FiberSlot* current = slots_.back();
        assert(!slots_.empty());
        slots_.pop_back();
        if (current->IsCons()) {
          slots_.insert(slots_.end(),
                        static_cast<const Cons*>(current)->begin(),
                        static_cast<const Cons*>(current)->end());
        } else {
          (*static_cast<Derived*>(this))(
              static_cast<const FiberBase*>(current));
          if (slots_.empty()) {
            break;
          }
        }
      }
    }

   private:
    trace::Vector<const FiberSlot*>::type slots_;
  };

  template<typename OutputIter>
  class Copier : public FiberEnumerator<Copier<OutputIter> > {
   public:
    typedef FiberEnumerator<Copier<OutputIter> > super_type;
    template<typename Iter>
    Copier(Iter it, Iter last, OutputIter out)
      : super_type(it, last), out_(out) { }

    OutputIter Copy() {
      super_type::Consume();
      return out_;
    }

    void operator()(const FiberBase* base) {
      out_ = base->Copy(out_);
    }
   private:
    OutputIter out_;
  };

  class Cons : public FiberSlot {
   public:
    friend class JSString;
    typedef Cons this_type;
    typedef const FiberSlot* value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef std::iterator_traits<iterator>::reference reference;
    typedef std::iterator_traits<const_iterator>::reference const_reference;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::iterator_traits<iterator>::difference_type difference_type;
    typedef std::size_t size_type;

    static std::size_t GetControlSize() {
      return IV_ROUNDUP(sizeof(this_type), sizeof(value_type));
    }

    const_reference operator[](size_type n) const {
      return (data())[n];
    }

    reference operator[](size_type n) {
      return (data())[n];
    }

    pointer data() {
      return reinterpret_cast<pointer>(this) +
          GetControlSize() / sizeof(value_type);
    }

    const_pointer data() const {
      return reinterpret_cast<const_pointer>(this) +
          GetControlSize() / sizeof(value_type);
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
      return begin() + fiber_count_;
    }

    const_iterator end() const {
      return begin() + fiber_count_;
    }

    const_iterator cend() const {
      return begin() + fiber_count_;
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

    Cons(std::size_t size, bool is_8bit, std::size_t fiber_count)
      : FiberSlot(size,
                  FiberSlot::IS_CONS |
                  ((is_8bit) ? FiberSlot::IS_8BIT : FiberSlot::NONE)),
        fiber_count_(fiber_count) {
    }

    static this_type* New(const JSString* lhs, const JSString* rhs) {
      this_type* cons = NewWithSize(lhs->size() + rhs->size(),
                                    lhs->Is8Bit() && rhs->Is8Bit(),
                                    lhs->fiber_count() + rhs->fiber_count());
      std::copy(
          lhs->fibers().begin(),
          lhs->fibers().begin() + lhs->fiber_count(),
          std::copy(rhs->fibers().begin(),
                    rhs->fibers().begin() + rhs->fiber_count(),
                    cons->begin()));
      return cons;
    }

    static this_type* NewWithSize(std::size_t size,
                                  bool is_8bit,
                                  std::size_t fiber_count) {
      void* mem = GC_MALLOC(GetControlSize() +
                            fiber_count * sizeof(value_type));
      return new (mem) Cons(size, is_8bit, fiber_count);
    }

    template<typename OutputIter>
    OutputIter Copy(OutputIter target) const {
      Copier<OutputIter> copier(begin(), end(), target);
      return copier.Copy();
    }

    size_type fiber_count() const {
      return fiber_count_;
    }

   private:
    size_type fiber_count_;
  };

 public:
  // iterators
  typedef FiberBase::const_iterator iterator;
  typedef FiberBase::const_iterator const_iterator;
  typedef FiberBase::const_reverse_iterator reverse_iterator;
  typedef FiberBase::const_reverse_iterator const_reverse_iterator;

  const_iterator begin() const { return Flatten()->cbegin(); }

  const_iterator cbegin() const { return begin(); }

  const_iterator end() const { return Flatten()->cend(); }

  const_iterator cend() const { return end(); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const { return rbegin(); }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const { return rend(); }

  // getters
  uint32_t size() const { return size_; }

  bool empty() const { return size() == 0; }

  bool Is8Bit() const { return is_8bit_; }

  bool Is16Bit() const { return !Is8Bit(); }

  std::size_t fiber_count() const { return fiber_count_; }

  const FiberSlots& fibers() const { return fibers_; }

  bool IsFlatten() const { return fiber_count() == 1 && !fibers_[0]->IsCons(); }

  inline const Fiber8* Get8Bit() const {
    assert(Is8Bit());
    return Flatten()->As8Bit();
  }

  inline const Fiber16* Get16Bit() const {
    assert(!Is8Bit());
    return Flatten()->As16Bit();
  }

  std::string GetUTF8() const {
    if (Is8Bit()) {
      std::string str;
      str.resize(size());
      Copy(str.begin());
      return str;
    } else {
      const Fiber16* fiber = Get16Bit();
      std::string str;
      str.reserve(size());
      if (core::unicode::UTF16ToUTF8(
              fiber->begin(), fiber->end(),
              std::back_inserter(str)) != core::unicode::UNICODE_NO_ERROR) {
        str.clear();
      }
      return str;
    }
  }

  core::UString GetUString() const {
    core::UString str;
    str.resize(size());
    Copy(str.begin());
    return str;
  }

  uint16_t At(size_type n) const {
    const FiberSlot* first = fibers_[fiber_count_ - 1];
    if (first->size() > n && !first->IsCons()) {
      return (*static_cast<const FiberBase*>(first))[n];
    }
    return (*Flatten())[n];
  }

  template<typename OutputIter>
  OutputIter Copy(size_type from, size_type to, OutputIter out) const {
    if (Is8Bit()) {
      const Fiber8* fiber = Get8Bit();
      return std::copy(fiber->begin() + from, fiber->begin() + to, out);
    } else {
      const Fiber16* fiber = Get16Bit();
      return std::copy(fiber->begin() + from, fiber->begin() + to, out);
    }
  }

  template<typename OutputIter>
  OutputIter Copy(OutputIter target) const {
    Copier<OutputIter> copier(fibers_.begin(),
                              fibers_.begin() + fiber_count(), target);
    return copier.Copy();
  }

  inline this_type* Repeat(Context* ctx, uint32_t count, Error* e) {
    if (count == 0 || empty()) {
      return this_type::NewEmptyString(ctx);
    }
    if (count == 1) {
      return this;
    }

    if (size() == 1) {
      // single character
      if (Is8Bit()) {
        const std::vector<char> vec(count, At(0));
        return this_type::New(ctx, vec.begin(), count, true, e);
      } else {
        const std::vector<uint16_t> vec(count, At(0));
        return this_type::New(ctx, vec.begin(), count, false, e);
      }
    }

    if (static_cast<uint64_t>(size()) * count > kMaxSize) {
      e->Report(Error::Type, "too long string is prohibited");
      return NULL;
    }

    return new this_type(ctx, this, count);
  }

  inline JSArray* Split(Context* ctx,
                        uint32_t limit, Error* e) const;

  inline JSArray* Split(Context* ctx,
                        uint16_t ch, uint32_t limit, Error* e) const;

  inline this_type* Substring(Context* ctx, uint32_t from, uint32_t to) const;

  inline size_type find(const this_type& target, size_type index) const;

  inline size_type rfind(const this_type& target, size_type index) const;

  int compare(const this_type& x) const {
    return Flatten()->compare(*x.Flatten());
  }

  friend bool operator==(const this_type& x, const this_type& y) {
    if (x.size() == y.size()) {
      return x.compare(y) == 0;
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

  int compare(const core::StringPiece& piece) const {
    if (Is8Bit()) {
      return core::StringPiece(*Get8Bit()).compare(piece);
    } else {
      const Fiber16* fiber = Get16Bit();
      return core::CompareIterators(fiber->begin(), fiber->end(), piece.begin(), piece.end());
    }
  }

  int compare(const core::UStringPiece& piece) const {
    if (Is16Bit()) {
      return core::UStringPiece(*Get16Bit()).compare(piece);
    } else {
      const Fiber8* fiber = Get8Bit();
      return core::CompareIterators(fiber->begin(), fiber->end(), piece.begin(), piece.end());
    }
  }

  // factory methods

  static this_type* New(Context* ctx, const core::UStringPiece& str, Error* e) {
    return New(ctx, str.begin(), str.size(), false, e);
  }

  static this_type* NewAsciiString(Context* ctx, const core::StringPiece& str, Error* e) {
    return New(ctx, str.begin(), str.size(), true, e);
  }

  static this_type* NewSingle(Context* ctx, uint16_t ch);

  template<typename Iter>
  static this_type* New(Context* ctx, Iter it, Iter last, bool is_8bit, Error * e) {
    return New(ctx, it, std::distance(it, last), is_8bit, e);
  }

  static this_type* NewEmptyString(Context* ctx);

  template<typename FiberType>
  static this_type* NewWithFiber(Context* ctx,
                                 const FiberType* fiber,
                                 std::size_t from, std::size_t to) {
    const std::size_t size = to - from;
    if (size == 0) {
      return NewEmptyString(ctx);
    }
    if (size == 1) {
      return NewSingle(ctx, (*fiber)[from]);
    }
    if (size == fiber->size()) {
      return new this_type(ctx, fiber);
    }
    return new this_type(ctx, fiber, from, to);
  }

  static this_type* New(Context* ctx, this_type* lhs, this_type* rhs, Error * e) {
    if (lhs->empty()) {
      return rhs;
    }

    if (rhs->empty()) {
      return lhs;
    }

    if (lhs->size() + rhs->size() > kMaxSize) {
      e->Report(Error::Type, "too long string is prohibited");
      return NULL;
    }

    return new this_type(ctx, lhs, rhs);
  }

  static this_type* New(Context* ctx, Symbol sym) {
    if (symbol::IsIndexSymbol(sym)) {
      const uint32_t index = symbol::GetIndexFromSymbol(sym);
      if (index < 10) {
        return NewSingle(ctx, index + '0');
      }
      std::array<char, 15> buffer;
      char* end = core::UInt32ToString(index, buffer.data());
      Error::Dummy dummy;
      return New(ctx, buffer.data(), end, true, &dummy);
    } else {
      assert(!symbol::IsIndexSymbol(sym));
      const core::UString* str = symbol::GetStringFromSymbol(sym);
      if (str->empty()) {
        return NewEmptyString(ctx);
      }
      if (str->size() == 1) {
        return NewSingle(ctx, (*str)[0]);
      }
      assert(str->size() <= kMaxSize);
      return new this_type(ctx, *str);
    }
  }

  static this_type* New(Context* ctx, JSVal* src, uint32_t count, Error* e) {
    size_type size = 0;
    size_type fiber_count = 0;
    bool is_8bit = true;
    for (uint32_t i = 0; i < count; ++i) {
      this_type* str = src[i].string();
      size += str->size();
      fiber_count += str->fiber_count();
      if (!str->Is8Bit()) {
        is_8bit = false;
      }
    }

    if (size > kMaxSize) {
      e->Report(Error::Type, "too long string is prohibited");
      return NULL;
    }

    return new this_type(ctx, src, count, size, fiber_count, is_8bit);
  }

  // destructor

  // TODO(Constellation) implement it
  void MarkChildren(radio::Core* core) { }

  static std::size_t SizeOffset() {
    return IV_OFFSETOF(JSString, size_);
  }

 private:
  template<typename Iter>
  static this_type* New(Context* ctx, Iter it, std::size_t n, bool is_8bit, Error* e) {
    if (n == 0) {
      return NewEmptyString(ctx);
    }
    if (n == 1) {
      return NewSingle(ctx, *it);
    }
    if (n > kMaxSize) {
      e->Report(Error::Type, "too long string is prohibited");
      return NULL;
    }
    return new this_type(ctx, it, n, is_8bit);
  }

  inline const FiberBase* Flatten() const {
    if (fiber_count_ != 1) {
      if (fiber_count_ == 2 && !fibers_[0]->IsCons() && !fibers_[1]->IsCons()) {
        FastFlatten(static_cast<const FiberBase*>(fibers_[1]),
                    static_cast<const FiberBase*>(fibers_[0]));
      } else {
        SlowFlatten();
      }
    } else if (fibers_[0]->IsCons()) {
      SlowFlatten();
    }
    assert(IsFlatten());
    return static_cast<const FiberBase*>(fibers_[0]);
  }

  template<typename FiberT>
  void FastFlattenImpl(const FiberBase* head, const FiberBase* tail) const {
    FiberT* fiber = FiberT::NewWithSize(size_);
    tail->Copy(head->Copy(fiber->begin()));
    // these are Fibers, not Cons. so simply call Release
    fiber_count_ = 1;
    fibers_[0] = fiber;
  }

  void FastFlatten(const FiberBase* head, const FiberBase* tail) const {
    // use fast case flatten
    // Fiber and Fiber
    if (Is8Bit()) {
      FastFlattenImpl<Fiber8>(head, tail);
    } else {
      FastFlattenImpl<Fiber16>(head, tail);
    }
  }

  template<typename FiberT>
  void SlowFlattenImpl() const {
    FiberT* fiber = FiberT::NewWithSize(size_);
    Copy(fiber->begin());
    fiber_count_ = 1;
    fibers_[0] = fiber;
  }

  void SlowFlatten() const {
    if (Is8Bit()) {
      SlowFlattenImpl<Fiber8>();
    } else {
      SlowFlattenImpl<Fiber16>();
    }
  }

  // constructors
  JSString(Context* ctx);
  explicit JSString(Context* ctx, const FiberBase* fiber);
  template<typename FiberType>
  JSString(Context* ctx, const FiberType* fiber, std::size_t from, std::size_t to);
  explicit JSString(Context* ctx, uint16_t ch);
  explicit JSString(Context* ctx, const core::UStringPiece& str);
  template<typename Iter>
  JSString(Context* ctx, Iter it, std::size_t n, bool is_8bit);
  JSString(Context* ctx, this_type* lhs, this_type* rhs);
  JSString(Context* ctx, this_type* target, uint32_t repeat);
  JSString(Context* ctx, JSVal* src, uint32_t count,
           size_type size, size_type fiber_count, bool is_8bit);

  uint32_t size_;
  bool is_8bit_;
  mutable std::size_t fiber_count_;
  mutable FiberSlots fibers_;
};

inline std::ostream& operator<<(std::ostream& os, const JSString& str) {
  if (str.Is8Bit()) {
    const Fiber8* fiber = str.Get8Bit();
    os.write(reinterpret_cast<const char*>(fiber->data()), fiber->size());
  } else {
    const Fiber16* fiber = str.Get16Bit();
    core::unicode::UTF16ToUTF8(fiber->begin(),
                               fiber->end(), std::ostream_iterator<char>(os));
  }
  return os;
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_FWD_H_
