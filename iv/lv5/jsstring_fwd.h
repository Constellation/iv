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
#include <iv/lv5/context_utils.h>
#include <iv/lv5/fiber.h>
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

class JSString: public radio::HeapObject<radio::STRING> {
 public:
  friend class GlobalData;
  typedef JSString this_type;

  // FiberSlots has FiberSlot by reverse order
  // for example, string "THIS" and "IS" to
  // [ "IS", "THIS", NULL, NULL, NULL ]
  typedef Fiber16::size_type size_type;

  static const size_type kMaxFibers = 5;
  static const size_type npos = static_cast<size_type>(-1);

  typedef std::array<FiberSlot*, kMaxFibers> FiberSlots;

 private:
  struct Retainer {
    FiberSlot* operator()(FiberSlot* slot) {
      slot->Retain();
      return slot;
    }
  };

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
    std::vector<const FiberSlot*> slots_;
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
    typedef FiberSlot* value_type;
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

   public:
    Cons(const JSString* lhs, const JSString* rhs, std::size_t fiber_count)
      : FiberSlot(lhs->size() + rhs->size(),
                  FiberSlot::IS_CONS |
                  ((lhs->Is8Bit() && rhs->Is8Bit()) ?
                   FiberSlot::IS_8BIT : FiberSlot::NONE)),
        fiber_count_(fiber_count) {
      // insert fibers by reverse order (rhs first)
      std::transform(
          lhs->fibers().begin(),
          lhs->fibers().begin() + lhs->fiber_count(),
          std::transform(rhs->fibers().begin(),
                         rhs->fibers().begin() + rhs->fiber_count(),
                         begin(), Retainer()),
          Retainer());
    }

    static this_type* New(const JSString* lhs, const JSString* rhs) {
      const std::size_t fiber_count = lhs->fiber_count() + rhs->fiber_count();
      void* mem = std::malloc(GetControlSize() +
                              fiber_count * sizeof(value_type));
      return new (mem) Cons(lhs, rhs, fiber_count);
    }

    template<typename OutputIter>
    OutputIter Copy(OutputIter target) const {
      Copier<OutputIter> copier(begin(), end(), target);
      return copier.Copy();
    }

    size_type fiber_count() const {
      return fiber_count_;
    }

    size_type fiber_count_;
  };

 public:

  std::size_t size() const { return size_; }

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

  uint16_t GetAt(size_type n) const {
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

  inline JSArray* Split(Context* ctx, JSArray* ary,
                        uint32_t limit, Error* e) const;

  inline JSArray* Split(Context* ctx, JSArray* ary,
                        uint16_t ch, uint32_t limit, Error* e) const;

  inline JSString* Substring(Context* ctx, uint32_t from, uint32_t to) const;

  inline size_type find(const JSString& target, size_type index) const;

  inline size_type rfind(const JSString& target, size_type index) const;

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

  static this_type* New(Context* ctx, const core::UStringPiece& str) {
    return New(ctx, str.begin(), str.size(), false);
  }

  static this_type* NewAsciiString(Context* ctx, const core::StringPiece& str) {
    return New(ctx, str.begin(), str.size(), true);
  }

  static this_type* NewSingle(Context* ctx, uint16_t ch) {
    if (this_type* res = context::LookupSingleString(ctx, ch)) {
      return res;
    }
    return new (PointerFreeGC) this_type(ch);
  }

  template<typename Iter>
  static this_type* New(Context* ctx,
                        Iter it, Iter last, bool is_8bit = false) {
    return New(ctx, it, std::distance(it, last), is_8bit);
  }

  static this_type* NewEmptyString(Context* ctx) {
    return context::EmptyString(ctx);
  }

  static this_type* New(Context* ctx, this_type* lhs, this_type* rhs) {
    if (lhs->empty()) {
      return rhs;
    } else if (rhs->empty()) {
      return lhs;
    } else {
      return new (PointerFreeGC) this_type(lhs, rhs);
    }
  }

  static this_type* New(Context* ctx, Symbol sym) {
    if (symbol::IsIndexSymbol(sym)) {
      const uint32_t index = symbol::GetIndexFromSymbol(sym);
      if (index < 10) {
        return NewSingle(ctx, index + '0');
      }
      std::array<char, 15> buffer;
      char* end = core::UInt32ToString(index, buffer.data());
      return New(ctx, buffer.data(), end, true);
    } else {
      return New(ctx, *symbol::GetStringFromSymbol(sym));
    }
  }

  ~JSString() {
    Destroy(fibers_.begin(), fibers_.begin() + fiber_count());
  }

  template<typename Iter>
  static void Destroy(Iter it, Iter last) {
    std::vector<FiberSlot*> slots(it, last);
    while (true) {
      FiberSlot* current = slots.back();
      assert(!slots.empty());
      slots.pop_back();
      if (current->IsCons() && current->RetainCount() == 1) {
        slots.insert(slots.end(),
                     static_cast<Cons*>(current)->begin(),
                     static_cast<Cons*>(current)->end());
      }
      current->Release();
      if (slots.empty()) {
        break;
      }
    }
  }

  void MarkChildren(radio::Core* core) { }

 private:
  template<typename Iter>
  static this_type* New(Context* ctx, Iter it, std::size_t n, bool is_8bit) {
    if (n <= 1) {
      if (n == 0) {
        return NewEmptyString(ctx);
      } else {
        return NewSingle(ctx, *it);
      }
    }
    return new (PointerFreeGC) this_type(it, n, is_8bit);
  }

  inline const FiberBase* Flatten() const {
    if (fiber_count_ != 1) {
      if (fiber_count_ == 2 && !fibers_[0]->IsCons() && !fibers_[1]->IsCons()) {
        FastFlatten(static_cast<FiberBase*>(fibers_[1]),
                    static_cast<FiberBase*>(fibers_[0]));
      } else {
        SlowFlatten();
      }
    } else if (fibers_[0]->IsCons()) {
      SlowFlatten();
    }
    assert(IsFlatten());
    return static_cast<const FiberBase*>(fibers_[0]);
  }

  template<typename CharT>
  void FastFlattenImpl(FiberBase* head, FiberBase* tail) const {
    Fiber<CharT>* fiber = Fiber<CharT>::NewWithSize(size_);
    tail->Copy(head->Copy(fiber->begin()));
    // these are Fibers, not Cons. so simply call Release
    head->Release();
    tail->Release();
    fiber_count_ = 1;
    fibers_[0] = fiber;
  }

  void FastFlatten(FiberBase* head, FiberBase* tail) const {
    // use fast case flatten
    // Fiber and Fiber
    if (Is8Bit()) {
      FastFlattenImpl<char>(head, tail);
    } else {
      FastFlattenImpl<uint16_t>(head, tail);
    }
  }

  template<typename CharT>
  void SlowFlattenImpl() const {
    Fiber<CharT>* fiber = Fiber<CharT>::NewWithSize(size_);
    Copy(fiber->begin());
    Destroy(fibers_.begin(), fibers_.begin() + fiber_count());
    fiber_count_ = 1;
    fibers_[0] = fiber;
  }

  void SlowFlatten() const {
    if (Is8Bit()) {
      SlowFlattenImpl<char>();
    } else {
      SlowFlattenImpl<uint16_t>();
    }
  }

  // empty string
  JSString()
    : size_(0),
      is_8bit_(true),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Fiber8::NewWithSize(0);
  }

  // single char string
  explicit JSString(uint16_t ch)
    : size_(1),
      is_8bit_(core::character::IsASCII(ch)),
      fiber_count_(1),
      fibers_() {
    if (Is8Bit()) {
      fibers_[0] = Fiber8::New(&ch, 1);
    } else {
      fibers_[0] = Fiber16::New(&ch, 1);
    }
  }

  template<typename Iter>
  JSString(Iter it, std::size_t n, bool is_8bit)
    : size_(n),
      is_8bit_(is_8bit),
      fiber_count_(1),
      fibers_() {
    if (Is8Bit()) {
      fibers_[0] = Fiber8::New(it, size_);
    } else {
      fibers_[0] = Fiber16::New(it, size_);
    }
  }

  JSString(JSString* lhs, JSString* rhs)
    : size_(lhs->size() + rhs->size()),
      is_8bit_(lhs->Is8Bit() && rhs->Is8Bit()),
      fiber_count_(lhs->fiber_count_ + rhs->fiber_count_),
      fibers_() {
    if (fiber_count() <= kMaxFibers) {
      // flatten version
      fiber_count_ = 1;
      fibers_[0] = Cons::New(lhs, rhs);
      assert(Is8Bit() == fibers_[0]->Is8Bit());
    } else {
      assert(fiber_count_ <= kMaxFibers);
      // insert fibers by reverse order (rhs first)
      std::transform(
          lhs->fibers().begin(),
          lhs->fibers().begin() + lhs->fiber_count(),
          std::transform(rhs->fibers().begin(),
                         rhs->fibers().begin() + rhs->fiber_count(),
                         fibers_.begin(), Retainer()),
          Retainer());
    }
  }

  std::size_t size_;
  bool is_8bit_;
  mutable std::size_t fiber_count_;
  mutable FiberSlots fibers_;
};

inline std::ostream& operator<<(std::ostream& os, const JSString& str) {
  return os << str.GetUTF8();
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_FWD_H_
