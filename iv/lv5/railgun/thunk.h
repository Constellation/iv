#ifndef IV_LV5_RAILGUN_THUNK_H_
#define IV_LV5_RAILGUN_THUNK_H_
#include <vector>
#include <iv/lv5/railgun/register_id.h>
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline void ThunkPool::Initialize(Code* code) {
  parameter_offset_ = code->params().size();
  vec_.assign(parameter_offset_ + code->stack_size(), NULL);
}

inline int32_t ThunkPool::CalculateOffset(int16_t register_offset) {
  // local variable
  if (register_offset >= 0) {
    return register_offset + parameter_offset_;
  }

  // this register
  if (register_offset == FrameConstant<>::kThisOffset) {
    return -1;
  }

  // parameter
  return FrameConstant<>::ConvertRegisterToArg(register_offset);
}

inline Thunk* ThunkPool::Lookup(int16_t register_offset) {
  const int32_t offset = CalculateOffset(register_offset);
  if (offset < 0) {
    return NULL;
  }
  assert(static_cast<uint32_t>(offset) < vec_.size());
  return vec_[offset];
}

inline void ThunkPool::Insert(int16_t register_offset, Thunk* thunk) {
  const int32_t offset = CalculateOffset(register_offset);
  if (offset < 0) {
    return;
  }
  assert(static_cast<uint32_t>(offset) < vec_.size());
  vec_[offset] = thunk;
}

inline void ThunkPool::Push(Thunk* thunk) {
  Thunk* target = Lookup(thunk->register_offset());
  if (target) {
    thunk->next_ = target;
    target->prev_->next_ = thunk;
    thunk->prev_ = target->prev_;
    target->prev_ = thunk;
  } else {
    Insert(thunk->register_offset(), thunk);
  }
}

inline void ThunkPool::Remove(Thunk* thunk) {
  if (!thunk->IsChained()) {
    assert(Lookup(thunk->register_offset()));
    Insert(thunk->register_offset(), NULL);
  } else {
    thunk->RemoveSelfFromChain();
  }
}

inline void ThunkPool::Spill(RegisterID reg) {
  if (reg->IsLocal()) {
    Thunk* start = Lookup(reg->register_offset());
    if (start) {
      Thunk* current = start;
      do {
        current->Spill();
        current = current->next_;
      } while (start != current);
      Insert(reg->register_offset(), NULL);
    }
  }
}

inline void ThunkPool::ForceSpill() {
  for (ThunkVector::const_iterator it = vec_.begin(),
       last = vec_.end(); it != last; ++it) {
    if (*it) {
      Thunk* start = *it;
      Thunk* current = start;
      do {
        current->Spill();
        current = current->next_;
      } while (start != current);
    }
  }
  vec_.assign(vec_.size(), NULL);
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
