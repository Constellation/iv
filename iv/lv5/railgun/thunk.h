#ifndef IV_LV5_RAILGUN_THUNK_H_
#define IV_LV5_RAILGUN_THUNK_H_
#include <vector>
#include <iv/lv5/railgun/register_id.h>
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline void ThunkList::Push(Thunk* thunk) {
  vec_.push_back(thunk);
}

inline void ThunkList::Remove(Thunk* thunk) {
  vec_.erase(std::find(vec_.begin(), vec_.end(), thunk));
}

struct ThunkSpiller {
  ThunkSpiller(RegisterID reg)
    : reg_(reg) {
  }

  bool operator()(Thunk* thunk) {
    return thunk->Spill(reg_);
  }

  RegisterID reg_;
};

inline void ThunkList::Spill(RegisterID reg) {
  if (reg->IsLocal()) {
    vec_.erase(
        std::remove_if(vec_.begin(),
                       vec_.end(), ThunkSpiller(reg)), vec_.end());
  }
}

inline void ThunkList::ForceSpill() {
  for (Thunks::const_iterator it = vec_.begin(),
       last = vec_.end(); it != last; ++it) {
    (*it)->ForceSpill();
  }
  vec_.clear();
}

inline RegisterID ThunkList::EmitMV(RegisterID local) {
  assert(local->IsLocal());
  return compiler_->SpillRegister(local);
}

inline Thunk::Thunk(ThunkList* list, RegisterID reg)
  : reg_(reg),
    list_(list),
    released_(false) {
  assert(list);
  assert(reg);
  if (reg->IsLocal()) {
    if (reg->IsHeap()) {
      ForceSpill();
    } else {
      list_->Push(this);
    }
  }
}

inline bool Thunk::Spill(RegisterID reg) {
  assert(reg_->IsLocal());
  if (reg == reg_) {
    // spill this register
    reg_ = list_->EmitMV(reg_);
    assert(!reg_->IsLocal());
    return true;
  }
  return false;
}

inline void Thunk::ForceSpill() {
  assert(reg_->IsLocal());
  reg_ = list_->EmitMV(reg_);
  assert(!reg_->IsLocal());
}

inline RegisterID Thunk::Release() {
  assert(reg_);
  if (!released_ && reg_->IsLocal()) {
    released_ = true;
    list_->Remove(this);
  }
  return reg_;
}

inline Thunk::~Thunk() {
  Release();
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_THUNK_H_
