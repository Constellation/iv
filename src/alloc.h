#ifndef _IV_ALLOC_H_
#define _IV_ALLOC_H_
#include <cstdlib>
#include <new>
#include <vector>
#include <algorithm>
#include <string>
#include <limits>
#include "uchar.h"
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

class Pool {
 public:
  static const unsigned int kPoolSize = Size::KB * 4;
  inline void Initialize(uintptr_t start, Pool* next) {
    start_ = start;
    position_ = start;
    limit_ = start + Pool::kPoolSize + 1;
    next_ = next;
  }
  Pool* Next() const { return next_; }
  inline void* New(uintptr_t size) {
    if ((position_ + size) < limit_) {
      // in this pool
      uintptr_t result = position_;
      position_ += size;
      return reinterpret_cast<void*>(result);
    }
    return NULL;
  }
 private:
  uintptr_t start_;
  uintptr_t position_;
  uintptr_t limit_;
  Pool* next_;
};

class Arena {
 public:
  static const unsigned int kPoolNum = 64;
  static const uintptr_t kAlignment = 8;
  static const unsigned int kArenaSize = (Pool::kPoolSize * kPoolNum) +
                                ((Pool::kPoolSize <= kAlignment) ?
                                  0 : (Pool::kPoolSize - kAlignment));
  Arena()
    : pools_(),
      result_(),
      now_(&pools_[0]),
      start_(now_),
      next_(NULL) {
    uintptr_t address = reinterpret_cast<uintptr_t>(result_);

    uintptr_t pool_address = AlignOffset(address, Pool::kPoolSize);
    unsigned int c = 0;
    for (; c < kPoolNum-1; ++c) {
      pools_[c].Initialize(pool_address+c*Pool::kPoolSize, &pools_[c+1]);
    }
    pools_[kPoolNum-1].Initialize(
        pool_address+(kPoolNum-1)*Pool::kPoolSize, NULL);
  }

  inline void* New(std::size_t raw_size) {
    uintptr_t size = AlignOffset(raw_size, kAlignment);
    void* result = now_->New(size);
    if (result == NULL) {
      now_ = now_->Next();
      while (now_) {
        result = now_->New(size);
        if (result == NULL) {
          now_ = now_->Next();
        } else {
          return result;
        }
      }
      return NULL;  // full up
    }
    return result;
  }
  inline void SetNext(Arena* next) { next_ = next; }
  inline Arena* Next() const { return next_; }
 private:
  Pool pools_[kPoolNum];
  char result_[kArenaSize];
  Pool* now_;
  const Pool* start_;
  Arena* next_;
};

template<std::size_t N>
class Space {
 public:
  Space()
    : arena_(&init_arenas_[0]),
      start_malloced_(NULL),
      malloced_() {
    for (std::size_t c = 0; c < N-1; ++c) {
      init_arenas_[c].SetNext(&init_arenas_[c+1]);
    }
  }
  ~Space() {
    if (start_malloced_) {
      Arena *now = start_malloced_, *next = start_malloced_;
      while (now) {
        next = now->Next();
        delete now;
        now = next;
      }
    }
    std::for_each(malloced_.begin(), malloced_.end(), &Malloced::Delete);
  }
  inline void* New(std::size_t size) {
    if ((size - 1) < kThreshold) {
      // small memory allocator
      void* result = arena_->New(size);
      if (result == NULL) {
        Arena *arena = arena_->Next();
        if (arena) {
          arena_ = arena;
        } else {
          NewArena();
          if (!start_malloced_) {
            start_malloced_ = arena_;
          }
        }
        result = arena_->New(size);
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

  inline void Clear() {
    arena_ = init_arenas_;
    std::for_each(malloced_.begin(), malloced_.end(), &Malloced::Delete);
    malloced_.clear();
  }

 private:
  static const std::size_t kThreshold = 256;
  static const std::size_t kInitArenas = N;

  inline Arena* NewArena() {
    Arena* arena = NULL;
    try {
      arena = new Arena();
      if (arena == NULL) {
        Malloced::OutOfMemory();
        return arena;
      }
    } catch(const std::bad_alloc&) {
      Malloced::OutOfMemory();
      return arena;
    }
    arena_->SetNext(arena);
    arena_ = arena;
    return arena;
  }
  inline const std::vector<void*>& malloced() const {
    return malloced_;
  }
  Arena init_arenas_[kInitArenas];
  Arena* arena_;
  Arena* start_malloced_;
  std::vector<void*>malloced_;
};

// surpress compiler warning
template<>
inline Space<1>::Space()
  : arena_(&init_arenas_[0]),
    start_malloced_(NULL),
    malloced_() {
}

} }  // namespace iv::core
#endif  // _IV_ALLOC_H_
