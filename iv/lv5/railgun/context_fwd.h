#ifndef IV_LV5_RAILGUN_CONTEXT_FWD_H_
#define IV_LV5_RAILGUN_CONTEXT_FWD_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/railgun/fwd.h>
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

  inline JSVal* StackGain(std::size_t size);

  inline void StackRelease(std::size_t size);

 private:
  VM* vm_;
};


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONTEXT_FWD_H_
