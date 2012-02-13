#ifndef IV_LV5_RAILGUN_CONTEXT_FWD_H_
#define IV_LV5_RAILGUN_CONTEXT_FWD_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/lru_code_map.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Context : public lv5::Context {
 public:
  explicit inline Context();
  inline ~Context();

  VM* vm() {
    return vm_;
  }

  LRUCodeMap* direct_eval_map() {
    return &direct_eval_map_;
  }
 private:
  VM* vm_;
  LRUCodeMap direct_eval_map_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONTEXT_FWD_H_
