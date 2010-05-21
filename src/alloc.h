#ifndef _IV_ALLOC_H_
#define _IV_ALLOC_H_
#include <new>
#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>
#include "utils.h"

namespace iv {
namespace core {

// Malloc
// memory allocated from std::malloc
class Malloced {
 public:
  void* operator new(std::size_t size) { return New(size); }
  void  operator delete(void* p) { Delete(p); }
  static void OutOfMemory();
  static void* New(std::size_t size);
  static void Delete(void* p);
};

class Pool {
 public:
  static const unsigned int kPoolSize = Size::KB * 4;
  Pool();
  void Initialize(uintptr_t start, Pool* next);
  Pool* Next() const { return next_; }
  void* New(uintptr_t size);
 private:
  uintptr_t start_;
  uintptr_t position_;
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
  void* New(std::size_t raw_size);
  void SetNext(Arena* next) { next_ = next; }
  Arena* Next() const { return next_; }
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
  void* New(std::size_t size);

 private:
  static const std::size_t kThreshold = 256;
  static const unsigned int kInitArenas = 4;
  struct Freer {
    void operator()(void* p) const {
      Malloced::Delete(p);
    }
  };

  Arena* NewArena();

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
  explicit SpaceAllocator(const SpaceAllocator& alloc) throw() : space_(alloc.space_) { }
  template<class U>
  explicit SpaceAllocator(const SpaceAllocator<U>& alloc) throw() : space_(alloc.space_) { }

  ~SpaceAllocator() throw() {}

  pointer address(reference x) const {
    return &x;
  }

  const_pointer address(const_reference x) const {
    return &x;
  }

  pointer allocate(size_type n) {
    return reinterpret_cast<pointer>(space_->New(n * sizeof(T)));
  }

  void deallocate(pointer, size_type) { }

  size_type max_size() const {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  void construct(pointer p, const T& val) {
    new(reinterpret_cast<void*>(p)) T(val);
  }

  void destroy(pointer p) {
    (p)->~T();
  }

  char* _Charalloc(size_type n) {
    return allocate(n);
  }

  bool operator==(SpaceAllocator const&) { return true; }
  bool operator!=(SpaceAllocator const& a) { return !operator==(a); }
 private:
  Space* space_;
};

template<typename T>
struct SpaceVector {
  typedef std::vector<T, SpaceAllocator<T> > type;
};

} }  // namespace iv::core

#endif  // _IV_ALLOC_H_

