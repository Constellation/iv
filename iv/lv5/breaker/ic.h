#ifndef IV_BREAKER_IC_H_
#define IV_BREAKER_IC_H_
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

class IC {
 public:
  enum Type {
    MONO,
    POLY
  };

  explicit IC(Type type)
    : type_(type) {
  }

  virtual ~IC() { }

  virtual void MarkChildren(radio::Core* core) { }
  virtual GC_ms_entry* MarkChildren(GC_word* top,
                                    GC_ms_entry* entry,
                                    GC_ms_entry* mark_sp_limit,
                                    GC_word env) {
    return entry;
  }
 private:
  Type type_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
