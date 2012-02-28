#ifndef IV_LV5_RAILGUN_THUNK_H_
#define IV_LV5_RAILGUN_THUNK_H_
#include <vector>
#include <iv/lv5/railgun/register_id.h>
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline void ThunkPool::Push(Thunk* thunk) {
  const ThunkMap::const_iterator it = map_.find(thunk->register_offset());
  if (it != map_.end()) {
    thunk->next_ = it->second;
    it->second->prev_->next_ = thunk;
    thunk->prev_ = it->second->prev_;
    it->second->prev_ = thunk;
  } else {
    map_.insert(it, std::make_pair(thunk->register_offset(), thunk));
  }
}

inline void ThunkPool::Remove(Thunk* thunk) {
  if (!thunk->IsChained()) {
    assert(map_.find(thunk->register_offset()) != map_.end());
    map_.erase(thunk->register_offset());
  } else {
    thunk->RemoveSelfFromChain();
  }
}

inline void ThunkPool::Spill(RegisterID reg) {
  if (reg->IsLocal()) {
    const ThunkMap::iterator it = map_.find(reg->register_offset());
    if (it != map_.end()) {
      Thunk* start = it->second;
      Thunk* current = start;
      do {
        current->Spill();
        current = current->next_;
      } while (start != current);
      map_.erase(it);
    }
  }
}

inline void ThunkPool::ForceSpill() {
  for (ThunkMap::const_iterator it = map_.begin(),
       last = map_.end(); it != last; ++it) {
    Thunk* start = it->second;
    Thunk* current = start;
    do {
      current->Spill();
      current = current->next_;
    } while (start != current);
  }
  map_.clear();
}

inline RegisterID ThunkPool::EmitMV(RegisterID local) {
  assert(local->IsLocal());
  return compiler_->SpillRegister(local);
}

inline Thunk::Thunk(ThunkPool* pool, RegisterID reg)
  : reg_(reg),
    pool_(pool),
    prev_(this),
    next_(this),
    released_(false) {
  assert(pool);
  assert(reg);
  if (reg->IsHeap()) {
    Spill();
  } else if (reg->IsStack()) {
    pool_->Push(this);
  }
}

inline void Thunk::Spill() {
  // spill this register
  assert(reg_->IsLocal());
  reg_ = pool_->EmitMV(reg_);
  assert(!reg_->IsLocal());
}

inline void Thunk::RemoveSelfFromChain() {
  assert(IsChained());
  prev_->next_ = next_;
  next_->prev_ = prev_;
  next_ = prev_ = this;
}

inline RegisterID Thunk::Release() {
  assert(reg_);
  if (!released_ && reg_->IsLocal()) {
    released_ = true;
    pool_->Remove(this);
  }
  return reg_;
}

inline Thunk::~Thunk() {
  Release();
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_THUNK_H_
