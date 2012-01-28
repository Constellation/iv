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
  ThunkSpiller(ThunkList* list, RegisterID reg)
    : list_(list),
      reg_(reg) {
  }

  bool operator()(Thunk* thunk) {
    return thunk->Spill(list_, reg_);
  }

  ThunkList* list_;
  RegisterID reg_;
};

inline void ThunkList::Spill(RegisterID reg) {
  if (reg->IsLocal()) {
    table_.clear();
    vec_.erase(
        std::remove_if(vec_.begin(),
                       vec_.end(), ThunkSpiller(this, reg)), vec_.end());
  }
}

inline RegisterID ThunkList::EmitMV(RegisterID local) {
  assert(local->IsLocal());
  TransTable::const_iterator it = table_.find(local->reg());
  if (it != table_.end()) {
    return it->second;
  }
  RegisterID reg = compiler_->EmitMV(local);
  table_.insert(std::make_pair(local->reg(), reg));
  return reg;
}

inline Thunk::Thunk(ThunkList* list, RegisterID reg)
  : reg_(reg),
    list_(list) {
  assert(list);
  assert(reg);
  if (reg->IsLocal()) {
    list_->Push(this);
  }
}

inline bool Thunk::Spill(ThunkList* list, RegisterID reg) {
  if (reg_->IsLocal()) {
    if (reg == reg_) {
      // spill this register
      reg_ = list_->EmitMV(reg_);
      assert(!reg_->IsLocal());
      return true;
    }
  }
  return false;
}


inline Thunk::~Thunk() {
  if (reg_->IsLocal()) {
    list_->Remove(this);
  }
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_THUNK_H_
