#ifndef IV_LV5_RAILGUN_THUNK_FWD_H_
#define IV_LV5_RAILGUN_THUNK_FWD_H_
#include <vector>
#include <iv/lv5/railgun/register_id.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Compiler;
class Code;
class Thunk;

class ThunkPool : private core::Noncopyable<ThunkPool> {
 public:
  friend class Thunk;
  typedef std::unordered_map<int32_t, Thunk*> ThunkMap;
  typedef std::vector<Thunk*> ThunkVector;

  explicit ThunkPool(Compiler* compiler)
    : vec_(), parameter_offset_(), compiler_(compiler) { }

  void Spill(RegisterID reg);

  void ForceSpill();

  void Initialize(Code* code);

 private:
  int32_t CalculateOffset(int16_t register_offset);

  Thunk* Lookup(int16_t register_offset);

  void Insert(int16_t register_offset, Thunk* thunk);

  void Push(Thunk* thunk);

  void Remove(Thunk* thunk);

  RegisterID EmitMV(RegisterID local);

  ThunkVector vec_;
  int32_t parameter_offset_;
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
