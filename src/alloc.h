#ifndef _IV_ALLOC_H_
#define _IV_ALLOC_H_
#include <cassert>
#include <new>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <limits>
#include <functional>
#include <numeric>
#include <tr1/unordered_map>
#include <tr1/functional>
#include <unicode/uchar.h>
#include "utils.h"

namespace iv {
namespace core {

// Malloc
// memory allocated from std::malloc
class Malloced {
 public:
  inline void* operator new(std::size_t size) { return New(size); }
  void  operator delete(void* p) { Delete(p); }
  inline static void* New(std::size_t size);
  static void OutOfMemory();
  static inline void Delete(void* p) {
    std::free(p);
  }
};

class Pool {
 public:
  static const unsigned int kPoolSize = Size::KB * 4;
  Pool();
  inline void Initialize(uintptr_t start, Pool* next);
  Pool* Next() const { return next_; }
  inline void* New(uintptr_t size);
 private:
  uintptr_t start_;
  uintptr_t position_;
  uintptr_t limit_;
  Pool* next_;
};

class Arena {
 public:
  static const unsigned int kPoolNum = 64;
  static const uintptr_t kAlignment = Size::kPointerSize;
  static const unsigned int kArenaSize = (Pool::kPoolSize * kPoolNum) +
                                ((Pool::kPoolSize <= kAlignment) ?
                                  0 : (Pool::kPoolSize - kAlignment));
  Arena();
  ~Arena();
  inline void* New(std::size_t raw_size);
  inline void SetNext(Arena* next) { next_ = next; }
  inline Arena* Next() const { return next_; }
 private:
  Pool pools_[kPoolNum];
  void* result_;
  Pool* now_;
  const Pool* start_;
  Arena* next_;
};

class Space {
 public:
  Space();
  virtual ~Space();
  inline void* New(std::size_t size);
  virtual inline void Clear();

 private:
  static const std::size_t kThreshold = 256;
  static const unsigned int kInitArenas = 2;

  inline Arena* NewArena();
  Arena init_arenas_[kInitArenas];
  Arena* arena_;
  Arena* start_malloced_;
  std::vector<void*>malloced_;
};

class SpaceObject {
 public:
  void* operator new(std::size_t size, Space* factory) {
    return factory->New(size);
  }
  void operator delete(void*, std::size_t) {
    UNREACHABLE();
  }
};

template<class T>
class SpaceAllocator {
 public:
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;
  template<class U>
  struct rebind {
    typedef SpaceAllocator<U> other;
  };

  SpaceAllocator() : space_(NULL) { }
  explicit SpaceAllocator(Space* factory) throw() : space_(factory) { }

  template<class U>
  SpaceAllocator(const SpaceAllocator<U>& alloc) throw()  // NOLINT
    : space_(alloc.space()) {
  }

  ~SpaceAllocator() throw() {}

  inline pointer address(reference x) const {
    return &x;
  }

  inline const_pointer address(const_reference x) const {
    return &x;
  }

  inline pointer allocate(size_type n, const void* = 0) {
    assert(space_);
    return reinterpret_cast<pointer>(space_->New(n * sizeof(T)));
  }

  inline void deallocate(pointer, size_type) { }

  inline size_type max_size() const {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  inline void construct(pointer p, const T& val) {
    new(reinterpret_cast<void*>(p)) T(val);
  }

  inline void destroy(pointer p) {
    (p)->~T();
  }

  inline char* _Charalloc(size_type n) {
    return allocate(n);
  }

  template<typename Other>
  inline SpaceAllocator<T>& operator=(const SpaceAllocator<Other>& rhs) {
    if (this != &rhs) {
      SpaceAllocator<T>(rhs).Swap(*this);
    }
    return *this;
  }

  Space* space() const {
    return space_;
  }

 protected:
  void Swap(SpaceAllocator<T>& rhs) {
    using std::swap;
    swap(space_, rhs.space_);
  }
  Space* space_;

 private:
  void operator=(const SpaceAllocator&);
};

template <typename T>
bool operator==(const SpaceAllocator<T>& lhs, const SpaceAllocator<T>& rhs) {
  return true;
}

template <typename T>
bool operator!=(const SpaceAllocator<T>& lhs, const SpaceAllocator<T>& rhs) {
  return false;
}

template<typename T>
struct SpaceVector {
  typedef std::vector<T, SpaceAllocator<T> > type;
};

template<typename T1, typename T2>
struct SpaceMap {
  typedef std::map<T1,
                   T2,
                   std::less<T1>,
                   SpaceAllocator<std::pair<const T1, T2> > > type;
};

template<typename T1, typename T2>
struct SpaceHashMap {
  typedef std::tr1::unordered_map<T1,
                                  T2,
                                  std::tr1::hash<T1>,
                                  std::equal_to<T1>,
                                  SpaceAllocator<
                                    std::pair<const T1, T2> > > type;
};

typedef std::basic_string<UChar,
                          std::char_traits<UChar>,
                          SpaceAllocator<UChar> > SpaceUString;

} }  // namespace iv::core

namespace std {
namespace tr1 {

// template specialization for SpaceUString in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::core::SpaceUString>
  : public unary_function<iv::core::SpaceUString, std::size_t> {
  std::size_t operator()(const iv::core::SpaceUString& x) const {
    return std::accumulate(
        x.begin(),
        x.end(),
        0,
        std::tr1::bind(
            std::plus<std::size_t>(),
            std::tr1::bind(
                std::multiplies<std::size_t>(),
                std::tr1::placeholders::_1,
                131),
            std::tr1::placeholders::_2));
  }
};

} }  // namespace std::tr1
#endif  // _IV_ALLOC_H_
