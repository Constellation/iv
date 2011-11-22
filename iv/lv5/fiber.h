// + FiberSlot
//   + Fiber<uint16_t | char>
//       have string content
//       this class is published to world
//   + Cons
//       have Fiber array <Cons|Fiber>*
//       this class is only seen in JSString
#ifndef IV_LV5_FIBER_H_
#define IV_LV5_FIBER_H_
#include <cstdlib>
#include <iterator>
#include <iv/thread_safe_ref_counted.h>
#include <new>
namespace iv {
namespace lv5 {

class FiberSlot : public core::ThreadSafeRefCounted<FiberSlot> {
 public:
  typedef std::size_t size_type;

  inline void operator delete(void* p) {
    // this type memory is allocated by malloc
    if (p) {
      std::free(p);
    }
  }

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

template<typename CharT>
class Fiber : public FiberSlot {
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

} }  // namespace iv::lv5
#endif  // IV_LV5_FIBER_H_
