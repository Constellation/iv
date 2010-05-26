#ifndef IV_ALLOC_INL_H_
#define IV_ALLOC_INL_H_
#include "alloc.h"
#include <algorithm>
namespace iv {
namespace core {

inline void* Malloced::New(std::size_t size) {
  void* result = std::malloc(size);
  if (result == NULL) {
    OutOfMemory();
  }
  return result;
}

inline void* Space::New(std::size_t size) {
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

inline Arena* Space::NewArena() {
  Arena* arena = NULL;
  try {
    arena = new Arena();
    if (arena == NULL) {
      Malloced::OutOfMemory();
      return arena;
    }
  } catch(const std::bad_alloc& e) {
    Malloced::OutOfMemory();
    return arena;
  }
  arena_->SetNext(arena);
  arena_ = arena;
  return arena;
}

inline void Space::Clear() {
  arena_ = init_arenas_;
  std::for_each(malloced_.begin(), malloced_.end(), Freer());
  malloced_.clear();
}

inline void* Pool::New(uintptr_t size) {
  if ((position_ + size) < limit_) {
    // in this pool
    uintptr_t result = position_;
    position_ += size;
    return reinterpret_cast<void*>(result);
  }
  return NULL;
}

inline void* Arena::New(std::size_t raw_size) {
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

} }  // namespace iv::core
#endif  // IV_ALLOC_INL_H_

