#ifndef IV_LV5_RAILGUN_THUNK_FWD_H_
#define IV_LV5_RAILGUN_THUNK_FWD_H_
#include <vector>
#include <iv/lv5/railgun/register_id.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Compiler;
class Thunk;

class ThunkList : private core::Noncopyable<ThunkList> {
 public:
  typedef std::vector<Thunk*> Thunks;
  typedef std::unordered_map<uint32_t, RegisterID> TransTable;

  ThunkList(Compiler* compiler)
    : vec_(), table_(), compiler_(compiler) { }

  void Push(Thunk* thunk);

  void Remove(Thunk* thunk);

  void Spill(RegisterID reg);

  bool empty() const {
    return vec_.empty();
  }

  RegisterID EmitMV(RegisterID local);

 private:
  Thunks vec_;
  TransTable table_;
  Compiler* compiler_;
};

class Thunk : private core::Noncopyable<Thunk> {
 public:
  Thunk(ThunkList* list, RegisterID reg);

  bool Spill(ThunkList* list, RegisterID reg);

  RegisterID Release();

  RegisterID reg() const { return reg_; }

  ~Thunk();
 private:
  RegisterID reg_;
  ThunkList* list_;
  bool released_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_THUNK_FWD_H_
