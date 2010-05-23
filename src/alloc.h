#ifndef _IV_ALLOC_H_
#define _IV_ALLOC_H_
#include <new>
#include <vector>
#include <algorithm>
#include <limits>
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
  // using 8 (2^3) for alignment value. see Python GC code
  static const uintptr_t kAlignment = 8;
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
  static const unsigned int kInitArenas = 4;
  struct Freer {
    inline void operator()(void* p) const {
      Malloced::Delete(p);
    }
  };

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
    assert(false);
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

  explicit SpaceAllocator(Space* fac) throw() : space_(fac) { }
  explicit SpaceAllocator(const SpaceAllocator& alloc) throw()
    : space_(alloc.space_) {
  }
  template<class U>
  explicit SpaceAllocator(const SpaceAllocator<U>& alloc) throw()
    : space_(alloc.space_) {
  }

  ~SpaceAllocator() throw() {}

  inline pointer address(reference x) const {
    return &x;
  }

  inline const_pointer address(const_reference x) const {
    return &x;
  }

  inline pointer allocate(size_type n) {
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

  inline bool operator==(SpaceAllocator const&) { return true; }
  inline bool operator!=(SpaceAllocator const& a) { return !operator==(a); }
 private:
  Space* space_;
};

template<typename T>
struct SpaceVector {
  typedef std::vector<T, SpaceAllocator<T> > type;
};

} }  // namespace iv::core

#endif  // _IV_ALLOC_H_

