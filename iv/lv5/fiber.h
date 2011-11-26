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
#include <iterator>
#include <new>
#include <iv/thread_safe_ref_counted.h>
namespace iv {
namespace lv5 {

class FiberSlot : public core::ThreadSafeRefCounted<FiberSlot> {
 public:
  typedef std::size_t size_type;
  enum Flag {
    NONE = 0,
    IS_CONS = 1,
    IS_8BIT = 2
  };

  static const int kFlagShift = 2;

  inline void operator delete(void* p) {
    // this type memory is allocated by malloc
    if (p) {
      std::free(p);
    }
  }

  bool IsCons() const {
    return flags_ & IS_CONS;
  }

  bool Is8Bit() const {
    return flags_ & IS_8BIT;
  }

  size_type size() const {
    return size_;
  }

 protected:
  explicit FiberSlot(std::size_t n, int flags)
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

  size_type size_;
  int flags_;

 private:
  // noncopyable
  FiberSlot(const FiberSlot&);
  const FiberSlot& operator=(const FiberSlot&);
};

template<typename CharT>
class Fiber;

class FiberBase : public FiberSlot {
 public:
  typedef FiberBase this_type;
  FiberBase(std::size_t size, int flags) : FiberSlot(size, flags) { }
  template<typename OutputIter>
  inline OutputIter Copy(OutputIter out) const;

  inline uint16_t operator[](size_type n) const;

  inline int compare(const this_type& x) const;

  inline const Fiber<char>* As8Bit() const;

  inline const Fiber<uint16_t>* As16Bit() const;

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

  static const int k8BitFlag =
      std::is_same<CharT, char>::value ? FiberSlot::IS_8BIT : FiberSlot::NONE;

  static std::size_t GetControlSize() {
    return IV_ROUNDUP(sizeof(this_type), sizeof(char_type));
  }

 private:
  template<typename String>
  explicit Fiber(const String& piece)
    : FiberBase(piece.size(), k8BitFlag) {
    std::copy(piece.begin(), piece.end(), begin());
  }

  explicit Fiber(std::size_t n)
    : FiberBase(n, k8BitFlag) {
  }

  template<typename Iter>
  Fiber(Iter it, std::size_t n)
    : FiberBase(n, k8BitFlag) {
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
};

typedef Fiber<char> Fiber8;
typedef Fiber<uint16_t> Fiber16;

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

} }  // namespace iv::lv5
#endif  // IV_LV5_FIBER_H_
