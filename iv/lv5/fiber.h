//
// + FiberSlot
//   + FiberBase
//     + Fiber<uint16_t | char>
//         have string content
//         this class is published to world
//   + Cons (in JSString)
//       have Fiber array <Cons|Fiber>*
//       this class is only seen in JSString
//
#ifndef IV_LV5_FIBER_H_
#define IV_LV5_FIBER_H_
#include <cstdlib>
#include <new>
#include <iterator>
#include <iv/thread_safe_ref_counted.h>
namespace iv {
namespace lv5 {

class JSString;

class FiberSlot : public core::ThreadSafeRefCounted<FiberSlot> {
 public:
  typedef std::size_t size_type;
  enum Flag {
    NONE = 0,
    IS_CONS = 1,
    IS_8BIT = 2,
    IS_ORIGINAL = 4,
    IS_EXTERNAL = 8
  };

  static const uint32_t kFlagShift = 2;

  inline void operator delete(void* p) {
    // this type memory is allocated by malloc
    if (p) {
      std::free(p);
    }
  }

  bool IsCons() const { return flags_ & IS_CONS; }

  bool Is8Bit() const { return flags_ & IS_8BIT; }

  bool IsOriginal() const { return flags_ & IS_ORIGINAL; }

  bool IsExternal() const { return flags_ & IS_EXTERNAL; }

  bool ReleaseNeeded() const {
    return !(flags_ & (IS_ORIGINAL | IS_CONS | IS_EXTERNAL));
  }

  size_type size() const { return size_; }

  inline ~FiberSlot();

 protected:
  explicit FiberSlot(uint32_t n, uint32_t flags)
    : size_(n),
      flags_(flags) {
  }

  void set_8bit(bool val) {
    if (val) {
      flags_ |= IS_8BIT;
    } else {
      flags_ &= ~IS_8BIT;
    }
  }

  uint32_t size_;
  uint32_t flags_;

 private:
  // noncopyable
  FiberSlot(const FiberSlot&);
  const FiberSlot& operator=(const FiberSlot&);
};

template<typename CharT>
class Fiber;

typedef Fiber<char> Fiber8;
typedef Fiber<uint16_t> Fiber16;

class FiberBase : public FiberSlot {
 public:
  typedef FiberBase this_type;

 private:
  friend class JSString;
  template<typename T>
  class iterator_base
    : public std::iterator<std::random_access_iterator_tag, uint16_t> {
   public:
    typedef iterator_base<T> this_type;
    typedef std::iterator<std::random_access_iterator_tag, T> super_type;
    typedef typename super_type::pointer pointer;
    typedef const typename super_type::pointer const_pointer;
    typedef uint16_t reference;
    typedef uint16_t const_reference;
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
  FiberBase(uint32_t size, uint32_t flags, FiberBase* original)
    : FiberSlot(size, flags),
      original_(original) {
  }

  FiberBase* original() const { return original_; }

  template<typename OutputIter>
  inline OutputIter Copy(OutputIter out) const;

  inline uint16_t operator[](size_type n) const;

  inline int compare(const this_type& x) const;

  inline const Fiber8* As8Bit() const;

  inline const Fiber16* As16Bit() const;

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

 private:
  FiberBase* original_;
};

template<typename CharT>
class Fiber : public FiberBase {
 public:
  typedef Fiber this_type;
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
  typedef std::size_t size_type;

  static const uint32_t k8BitFlag =
      std::is_same<CharT, char>::value ? FiberSlot::IS_8BIT : FiberSlot::NONE;

  static std::size_t GetControlSize() {
    return IV_ROUNDUP(sizeof(this_type), sizeof(char_type));
  }

 private:
  template<typename String>
  explicit Fiber(const String& piece)
    : FiberBase(piece.size(), k8BitFlag | IS_ORIGINAL, this),
      ptr_(reinterpret_cast<pointer>(this) +
           GetControlSize() / sizeof(char_type)) {
    std::copy(piece.begin(), piece.end(), begin());
  }

  explicit Fiber(uint32_t n)
    : FiberBase(n, k8BitFlag | IS_ORIGINAL, this),
      ptr_(reinterpret_cast<pointer>(this) +
           GetControlSize() / sizeof(char_type)) {
  }

  template<typename Iter>
  Fiber(Iter it, uint32_t n)
    : FiberBase(n, k8BitFlag | IS_ORIGINAL, this),
      ptr_(reinterpret_cast<pointer>(this) +
           GetControlSize() / sizeof(char_type)) {
    std::copy(it, it + n, begin());
  }

  Fiber(const this_type* fiber, uint32_t from, uint32_t to)
    : FiberBase((to - from), k8BitFlag, fiber->original()),
      ptr_(fiber->ptr_ + from) {
    original()->Retain();
  }

  explicit Fiber(const core::BasicStringPiece<CharT>& str)
    : FiberBase(str.size(), k8BitFlag | IS_ORIGINAL | IS_EXTERNAL, this),
      // TODO(Constellation) avoid const_cast
      ptr_(const_cast<CharT*>(str.data())) {
  }

 public:
  template<typename String>
  static this_type* New(const String& piece) {
    return New(piece.begin(), piece.size());
  }

  static this_type* NewWithSize(uint32_t n) {
    void* mem = std::malloc(GetControlSize() + n * sizeof(char_type));
    return new (mem) Fiber(n);
  }

  template<typename Iter>
  static this_type* New(Iter it, Iter last) {
    return New(it, std::distance(it, last));
  }

  template<typename Iter>
  static this_type* New(Iter it, uint32_t n) {
    void* mem = std::malloc(GetControlSize() + n * sizeof(char_type));
    return new (mem) Fiber(it, n);
  }

  static this_type* NewWithExternal(const core::BasicStringPiece<CharT>& str) {
    return new (std::malloc(sizeof(this_type))) Fiber(str);
  }

  static this_type* New(const this_type* original, uint32_t from, uint32_t to) {
    return new (std::malloc(sizeof(this_type))) Fiber(original, from, to);
  }

  operator core::BasicStringPiece<char_type>() const {
    return core::BasicStringPiece<char_type>(data(), size());
  }

  const_reference operator[](size_type n) const {
    return (data())[n];
  }

  reference operator[](size_type n) {
    return (data())[n];
  }

  pointer data() {
    return ptr_;
  }

  const_pointer data() const {
    return ptr_;
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

  template<typename OutputIter>
  inline OutputIter Copy(OutputIter out) const {
    return std::copy(begin(), end(), out);
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

  char_type* ptr_;
};

template<typename OutputIter>
OutputIter FiberBase::Copy(OutputIter out) const {
  if (Is8Bit()) {
    return As8Bit()->Copy(out);
  } else {
    return As16Bit()->Copy(out);
  }
}

uint16_t FiberBase::operator[](size_type n) const {
  if (Is8Bit()) {
    return (*As8Bit())[n];
  } else {
    return (*As16Bit())[n];
  }
}

const Fiber8* FiberBase::As8Bit() const {
  assert(Is8Bit());
  return static_cast<const Fiber8*>(this);
}

const Fiber16* FiberBase::As16Bit() const {
  assert(!Is8Bit());
  return static_cast<const Fiber16*>(this);
}

int FiberBase::compare(const this_type& x) const {
  if (Is8Bit() == x.Is8Bit()) {
    // same type. use downcast
    if (Is8Bit()) {
      return As8Bit()->compare(*x.As8Bit());
    } else {
      return As16Bit()->compare(*x.As16Bit());
    }
  }
  if (Is8Bit()) {
    return core::CompareIterators(
        As8Bit()->begin(), As8Bit()->end(),
        x.As16Bit()->begin(), x.As16Bit()->end());
  } else {
    return core::CompareIterators(
        As16Bit()->begin(), As16Bit()->end(),
        x.As8Bit()->begin(), x.As8Bit()->end());
  }
}

FiberSlot::~FiberSlot() {
  if (ReleaseNeeded()) {
    static_cast<const FiberBase*>(this)->original()->Release();
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_FIBER_H_
