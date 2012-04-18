// Templates create static functions which is used in breaker.
// But we want codes which we can control machine code completely,
// so use Xbyak and generate templates code at runtime.
#ifndef IV_LV5_BREAKER_TEMPLATES_H_
#define IV_LV5_BREAKER_TEMPLATES_H_
#include <iv/singleton.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Templates
  : private core::Singleton<Templates>,
    private Xbyak::CodeGenerator {
 public:
  friend class core::Singleton<Templates>;

 private:
  Templates()
    : core::Singleton<Templates>(),
      Xbyak::CodeGenerator() {
  }

  ~Templates() { }
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TEMPLATES_H_
