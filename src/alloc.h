#ifndef IV_ALLOC_H_
#define IV_ALLOC_H_
#include <cstdlib>
#include <new>
#include <vector>
#include <algorithm>
#include <string>
#include <limits>
#include "detail/array.h"
#include "static_assert.h"
#include "utils.h"

namespace iv {
namespace core {

// Malloc
// memory allocated from std::malloc
class Malloced {
 public:
  inline void* operator new(std::size_t size) { return New(size); }
  void  operator delete(void* p) { Delete(p); }
  static inline void* New(std::size_t size) {
    void* result = std::malloc(size);
    if (result == NULL) {
      OutOfMemory();
    }
    return result;
  }
  static void OutOfMemory() {
    std::exit(EXIT_FAILURE);
  }
  static inline void Delete(void* p) {
    std::free(p);
  }
};

class Space;

class Arena : private Noncopyable<Arena> {
 public:
  friend class Space;
  typedef Arena this_type;
  static const std::size_t kArenaSize = Size::KB * 4;
  static const uintptr_t kAlignment = 8;  // double or 64bit ptr size

  explicit Arena(Arena* prev)
    : next_(NULL),
      position_() {
    position_ =
        reinterpret_cast<uintptr_t>(this) +
        IV_ROUNDUP(sizeof(this_type), kAlignment);
    assert((position_ % kAlignment) == 0);
    if (prev) {
      prev->next_ = this;
    }
  }

  inline void* Allocate(std::size_t raw_size) {
    const std::size_t size = AlignOffset(raw_size, kAlignment);
    if ((position_ + size) <=
        (reinterpret_cast<uintptr_t>(this) + kArenaSize)) {
      // in this pool
      const uintptr_t result = position_;
      assert((result % kAlignment) == 0);
      position_ += size;
      return reinterpret_cast<void*>(result);
    }
    return NULL;  // full up
  }

  static Arena* New() {
    return new (Malloced::New(kArenaSize)) Arena(NULL);
  }

  static Arena* New(Arena* prev) {
    return new (Malloced::New(kArenaSize)) Arena(prev);
  }

  inline Arena* Next() const {
    return next_;
  }

  void ToStart() {
    position_ =
        reinterpret_cast<uintptr_t>(this) +
        IV_ROUNDUP(sizeof(this_type), kAlignment);
  }

 private:
  Arena* next_;
  uintptr_t position_;
};

class Space : private Noncopyable<Space> {
 public:
  Space()
    : arena_(Arena::New()),
      start_(arena_),
      malloced_() {
  }

  void Clear() {
    arena_ = start_;
    std::for_each(malloced_.begin(), malloced_.end(), &Malloced::Delete);
  }

  ~Space() {
    Arena *now = start_, *next = NULL;
    do {
      next = now->Next();
      Malloced::Delete(now);
      now = next;
    } while (now);
    std::for_each(malloced_.begin(), malloced_.end(), &Malloced::Delete);
  }

  inline void* New(std::size_t size) {
    if ((size - 1) < kThreshold) {
      // small memory allocator
      void* result = arena_->Allocate(size);
      if (result == NULL) {
        arena_->ToStart();
        Arena* arena = arena_->Next();
        if (arena) {
          arena_ = arena;
        } else {
          arena_ = Space::NewArena(arena_);
        }
        result = arena_->Allocate(size);
        if (result == NULL) {
          Malloced::OutOfMemory();
        }
      }
      return result;
    } else {
      // malloc as memory allocator
      void* result = Malloced::New(size);
      malloced_.push_back(result);
      return result;
    }
  }

 private:
  static const std::size_t kThreshold = Size::kPointerSize * 64;

  static Arena* NewArena(Arena* prev) {
    Arena* arena = NULL;
    arena = Arena::New(prev);
    if (arena == NULL) {
      Malloced::OutOfMemory();
      return arena;
    }
    return arena;
  }

  Arena* arena_;
  Arena* start_;
  std::vector<void*> malloced_;
};

} }  // namespace iv::core
#endif  // IV_ALLOC_H_
