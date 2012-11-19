#ifndef IV_LV5_I18N_COLLATOR_H_
#define IV_LV5_I18N_COLLATOR_H_
#include <iv/i18n.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/bind.h>
#include <iv/lv5/context_fwd.h>
#include <iv/lv5/i18n/utility.h>
namespace iv {
namespace lv5 {
namespace i18n {

class JSIntl : public JSObject {
 public:
  template<typename T>
  class GCHandle : public gc_cleanup {
   public:
    core::unique_ptr<T> handle;
  };
};

} } }  // namespace iv::lv5::i18n
#endif  // IV_LV5_I18N_COLLATOR_H_
