#include <cstdio>
#include <cstdlib>
#include "alloc-inl.h"

namespace iv {
namespace core {

void Malloced::OutOfMemory() {
  std::puts("FAIL");
  std::exit(EXIT_FAILURE);
}

Space::Space()
  : arena_(&init_arenas_[0]),
    start_malloced_(NULL),
    malloced_() {
  unsigned int c = 0;
  for (; c < kInitArenas-1; ++c) {
    init_arenas_[c].SetNext(&init_arenas_[c+1]);
  }
}

Space::~Space() {
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

void Pool::Initialize(uintptr_t start, Pool* next) {
  start_ = start;
  position_ = start;
  limit_ = start + Pool::kPoolSize + 1;
  next_ = next;
}

Arena::Arena()
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

} }  // namespace iv::core
