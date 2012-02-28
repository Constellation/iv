#ifndef IV_LV5_RAILGUN_THUNK_FWD_H_
#define IV_LV5_RAILGUN_THUNK_FWD_H_
#include <vector>
#include <iv/lv5/railgun/register_id.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Compiler;
class Thunk;

class ThunkPool : private core::Noncopyable<ThunkPool> {
 public:
  friend class Thunk;
  typedef std::unordered_map<int32_t, Thunk*> ThunkMap;

  explicit ThunkPool(Compiler* compiler)
    : map_(), compiler_(compiler) { }

  void Spill(RegisterID reg);

  void ForceSpill();

  bool empty() const {
    return map_.empty();
  }

 private:
  void Push(Thunk* thunk);

  void Remove(Thunk* thunk);

  RegisterID EmitMV(RegisterID local);

  ThunkMap map_;
  Compiler* compiler_;
};

class Thunk : private core::Noncopyable<Thunk> {
 public:
  friend class ThunkPool;
  Thunk(ThunkPool* pool, RegisterID reg);
 
  RegisterID Release();

  RegisterID reg() const { return reg_; }

  ~Thunk();

 private:
  bool IsChained() const {
    return next_ != this;
  }

  void Spill();

  int32_t register_offset() const { return reg_->register_offset(); }

  void RemoveSelfFromChain();

  RegisterID reg_;
  ThunkPool* pool_;
  Thunk* prev_;
  Thunk* next_;
  bool released_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_THUNK_FWD_H_
