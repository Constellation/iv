#ifndef IV_LV5_JSSTRING_FWD_H_
#define IV_LV5_JSSTRING_FWD_H_
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <vector>
#include <new>
#include <functional>
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include "detail/cstdint.h"
#include "detail/cinttypes.h"
#include "detail/memory.h"
#include "debug.h"
#include "unicode.h"
#include "noncopyable.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "static_assert.h"
#include "thread_safe_ref_counted.h"
#include "lv5/context_utils.h"
#include "lv5/radio/cell.h"
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
 private:
  struct Releaser {
    template<typename T>
    void operator()(T* ptr) {
      ptr->Release();
    }
  };

 public:
  class FiberSlot : public core::ThreadSafeRefCounted<FiberSlot> {
   public:
    typedef std::size_t size_type;

    inline void operator delete(void* p) {
      // this type memory is allocated by malloc
      if (p) {
        std::free(p);
      }
    }

    inline ~FiberSlot();

    bool IsCons() const {
      return is_cons_;
    }

    size_type size() const {
      return size_;
    }

   protected:
    explicit FiberSlot(std::size_t n, bool is_cons)
      : size_(n),
        is_cons_(is_cons) {
    }

    size_type size_;
    bool is_cons_;
  };

  class Fiber : public FiberSlot {
   public:
    typedef Fiber this_type;
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

    static std::size_t GetControlSize() {
      return IV_ROUNDUP(sizeof(this_type), sizeof(char_type));
    }

   private:
    template<typename String>
    explicit Fiber(const String& piece)
      : FiberSlot(piece.size(), false) {
      std::copy(piece.begin(), piece.end(), begin());
    }

    explicit Fiber(std::size_t n)
      : FiberSlot(n, false) {
    }

    template<typename Iter>
    Fiber(Iter it, std::size_t n)
      : FiberSlot(n, false) {
      std::copy(it, it + n, begin());
    }

   public:

    template<typename String>
    static this_type* New(const String& piece) {
      void* mem = std::malloc(GetControlSize() +
                              piece.size() * sizeof(char_type));
      return new (mem) Fiber(piece);
    }

    static this_type* NewWithSize(std::size_t n) {
      void* mem = std::malloc(GetControlSize() + n * sizeof(char_type));
      return new (mem) Fiber(n);
    }

    template<typename Iter>
    static this_type* New(Iter it, Iter last) {
      return New(it, std::distance(it, last));
    }

    template<typename Iter>
    static this_type* New(Iter it, std::size_t n) {
      void* mem = std::malloc(GetControlSize() + n * sizeof(char_type));
      return new (mem) Fiber(it, n);
    }

    bool IsCons() const {
      return false;
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
      return reinterpret_cast<pointer>(this) +
          GetControlSize() / sizeof(char_type);
    }

    const_pointer data() const {
      return reinterpret_cast<const_pointer>(this) +
          GetControlSize() / sizeof(char_type);
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
  };

 private:
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

    bool IsCons() const {
      return true;
    }

    Cons(const JSString* lhs, const JSString* rhs, std::size_t fiber_count)
      : FiberSlot(lhs->size() + rhs->size(), true),
        fiber_count_(fiber_count) {
      // insert fibers by reverse order (rhs first)
      iterator target = begin();
      for (FiberSlots::iterator it = rhs->fibers_.begin(),
           last = rhs->fibers_.begin() + rhs->fiber_count();
           it != last; ++it, ++target) {
        (*it)->Retain();
        *target = *it;
      }
      for (FiberSlots::iterator it = lhs->fibers_.begin(),
           last = lhs->fibers_.begin() + lhs->fiber_count();
           it != last; ++it, ++target) {
        (*it)->Retain();
        *target = *it;
      }
      assert(target == end());
    }

    static inline void Destroy(Cons* cons) {
      std::for_each(cons->begin(), cons->end(), Releaser());
    }

    static this_type* New(const JSString* lhs, const JSString* rhs) {
      const std::size_t fiber_count = lhs->fiber_count() + rhs->fiber_count();
      void* mem = std::malloc(GetControlSize() +
                              fiber_count * sizeof(value_type));
      return new (mem) Cons(lhs, rhs, fiber_count);
    }

    template<typename OutputIter>
    OutputIter Copy(OutputIter target) const {
      std::vector<const FiberSlot*> slots(begin(), end());
      while (true) {
        const FiberSlot* current = slots.back();
        assert(!slots.empty());
        slots.pop_back();
        if (current->IsCons()) {
          slots.insert(slots.end(),
                       static_cast<const Cons*>(current)->begin(),
                       static_cast<const Cons*>(current)->end());
        } else {
          target = std::copy(
              static_cast<const Fiber*>(current)->begin(),
              static_cast<const Fiber*>(current)->end(),
              target);
          if (slots.empty()) {
            break;
          }
        }
      }
      return target;
    }

    size_type fiber_count() const {
      return fiber_count_;
    }

    size_type fiber_count_;
  };

 public:
  friend class Cons;
  friend class GlobalData;
  typedef JSString this_type;

  // FiberSlots has FiberSlot by reverse order
  // for example, string "THIS" and "IS" to
  // [ "IS", "THIS", NULL, NULL, NULL ]
  static const std::size_t kMaxFibers = 5;
  typedef std::array<FiberSlot*, kMaxFibers> FiberSlots;
  typedef Fiber::size_type size_type;

  struct FlattenTag { };

  std::size_t size() const {
    return size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  inline void Flatten() const {
    if (fiber_count_ != 1) {
      if (fiber_count_ == 2 && !fibers_[0]->IsCons() && !fibers_[1]->IsCons()) {
        // use fast case flatten
        // Fiber and Fiber
        Fiber* fiber = Fiber::NewWithSize(size_);
        Fiber* head = static_cast<Fiber*>(fibers_[1]);
        Fiber* tail = static_cast<Fiber*>(fibers_[0]);
        std::copy(
            tail->begin(),
            tail->end(),
            std::copy(
                head->begin(),
                head->end(),
                fiber->begin()));
        head->Release();
        tail->Release();
        fiber_count_ = 1;
        fibers_[0] = fiber;
      } else {
        SlowFlatten();
      }
    } else if (fibers_[0]->IsCons()) {
      SlowFlatten();
    }
    assert(fibers_[0]->size() == size());
    assert(!fibers_[0]->IsCons());
  }

  inline const Fiber* GetFiber() const {
    Flatten();
    return static_cast<const Fiber*>(fibers_[0]);
  }

  std::string GetUTF8() const {
    const Fiber* fiber = GetFiber();
    std::string str;
    str.reserve(size());
    if (core::unicode::UTF16ToUTF8(
          fiber->begin(), fiber->end(),
          std::back_inserter(str)) != core::unicode::UNICODE_NO_ERROR) {
      str.clear();
    }
    return str;
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
      return (*static_cast<const Fiber*>(first))[n];
    }
    return (*GetFiber())[n];
  }

  template<typename Target>
  void CopyToString(Target* target) const {
    if (!empty()) {
      const Fiber* fiber = GetFiber();
      target->assign(fiber->data(), fiber->size());
    } else {
      target->assign(0UL, typename Target::value_type());
    }
  }

  template<typename Target>
  void AppendToString(Target* target) const {
    if (!empty()) {
      const Fiber* fiber = GetFiber();
      target->append(fiber->data(), fiber->size());
    }
  }

  template<typename OutputIter>
  OutputIter Copy(OutputIter target) const {
    std::vector<const FiberSlot*> slots(fibers_.begin(),
                                        fibers_.begin() + fiber_count());
    while (true) {
      const FiberSlot* current = slots.back();
      assert(!slots.empty());
      slots.pop_back();
      if (current->IsCons()) {
        slots.insert(slots.end(),
                     static_cast<const Cons*>(current)->begin(),
                     static_cast<const Cons*>(current)->end());
      } else {
        target = std::copy(
            static_cast<const Fiber*>(current)->begin(),
            static_cast<const Fiber*>(current)->end(),
            target);
        if (slots.empty()) {
          break;
        }
      }
    }
    return target;
  }

  inline JSArray* Split(Context* ctx, JSArray* ary,
                        uint32_t limit, Error* e) const;
  inline JSArray* Split(Context* ctx, JSArray* ary,
                        uint16_t ch, uint32_t limit, Error* e) const;

  friend bool operator==(const this_type& x, const this_type& y) {
    if (x.size() == y.size()) {
      return (*x.GetFiber()) == (*y.GetFiber());
    }
    return false;
  }

  friend bool operator!=(const this_type& x, const this_type& y) {
    return !(x == y);
  }

  friend bool operator<(const this_type& x, const this_type& y) {
    return (*x.GetFiber()) < (*y.GetFiber());
  }

  friend bool operator>(const this_type& x, const this_type& y) {
    return (*x.GetFiber()) > (*y.GetFiber());
  }

  friend bool operator<=(const this_type& x, const this_type& y) {
    return (*x.GetFiber()) <= (*y.GetFiber());
  }

  friend bool operator>=(const this_type& x, const this_type& y) {
    return (*x.GetFiber()) >= (*y.GetFiber());
  }

  static this_type* New(Context* ctx, const core::StringPiece& str) {
    std::vector<uint16_t> buffer;
    buffer.reserve(str.size());
    if (core::unicode::UTF8ToUTF16(
            str.begin(),
            str.end(),
            std::back_inserter(buffer)) != core::unicode::UNICODE_NO_ERROR) {
      buffer.clear();
    }
    return New(ctx, buffer.begin(), buffer.size());
  }

  static this_type* New(Context* ctx, const core::UStringPiece& str) {
    return New(ctx, str.begin(), str.size());
  }

  static this_type* NewAsciiString(Context* ctx,
                                   const core::StringPiece& str) {
    return New(ctx, str.begin(), str.size());
  }

  static this_type* NewSingle(Context* ctx, uint16_t ch) {
    if (this_type* res = context::LookupSingleString(ctx, ch)) {
      return res;
    }
    return new (PointerFreeGC) this_type(ch);
  }

  template<typename Iter>
  static this_type* New(Context* ctx, Iter it, Iter last) {
    return New(ctx, it, std::distance(it, last));
  }

  template<typename Iter>
  static this_type* New(Context* ctx, Iter it, std::size_t n) {
    if (n <= 1) {
      if (n == 0) {
        return NewEmptyString(ctx);
      } else {
        return NewSingle(ctx, *it);
      }
    }
    return new (PointerFreeGC) this_type(it, n);
  }

  static this_type* NewEmptyString(Context* ctx) {
    return context::EmptyString(ctx);
  }

  static this_type* New(Context* ctx, this_type* lhs, this_type* rhs) {
    if (lhs->empty()) {
      return rhs;
    } else if (rhs->empty()) {
      return lhs;
    } else if ((lhs->fiber_count_ + rhs->fiber_count_) <= kMaxFibers) {
      return new (PointerFreeGC) this_type(lhs, rhs);
    } else {
      // flatten version
      return new (PointerFreeGC) this_type(lhs, rhs, FlattenTag());
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
      return New(ctx, buffer.data(), end);
    } else {
      return New(ctx, *symbol::GetStringFromSymbol(sym));
    }
  }

  ~JSString() {
    std::for_each(fibers_.begin(), fibers_.begin() + fiber_count(), Releaser());
  }

 private:
  void SlowFlatten() const {
    Fiber* fiber = Fiber::NewWithSize(size_);
    Copy(fiber->begin());
    std::for_each(fibers_.begin(), fibers_.begin() + fiber_count(), Releaser());
    fiber_count_ = 1;
    fibers_[0] = fiber;
  }

  std::size_t fiber_count() const {
    return fiber_count_;
  }

  // empty string
  JSString()
    : size_(0),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Fiber::NewWithSize(0);
  }

  // single char string
  explicit JSString(uint16_t ch)
    : size_(1),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Fiber::New(&ch, 1);
  }

  template<typename Iter>
  JSString(Iter it, std::size_t n)
    : size_(n),
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
    // reverse order
    // rhs first
    FiberSlots::iterator target = fibers_.begin();
    for (FiberSlots::iterator it = rhs->fibers_.begin(),
         last = rhs->fibers_.begin() + rhs->fiber_count();
         it != last; ++it, ++target) {
      (*it)->Retain();
      *target = *it;
    }
    for (FiberSlots::iterator it = lhs->fibers_.begin(),
         last = lhs->fibers_.begin() + lhs->fiber_count();
         it != last; ++it, ++target) {
      (*it)->Retain();
      *target = *it;
    }
  }

  // flatten version
  JSString(JSString* lhs, JSString* rhs, FlattenTag tag)
    : size_(lhs->size() + rhs->size()),
      fiber_count_(1),
      fibers_() {
    fibers_[0] = Cons::New(lhs, rhs);
  }

  std::size_t size_;
  mutable std::size_t fiber_count_;
  mutable FiberSlots fibers_;
};

JSString::FiberSlot::~FiberSlot() {
  if (IsCons()) {
    Cons::Destroy(static_cast<Cons*>(this));
  }
}

inline std::ostream& operator<<(std::ostream& os, const JSString& str) {
  return os << str.GetUTF8();
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_FWD_H_
