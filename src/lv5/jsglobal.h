#ifndef _IV_LV5_JSGLOBAL_H_
#define _IV_LV5_JSGLOBAL_H_
#include "lv5/jsobject.h"
#include "lv5/class.h"
namespace iv {
namespace lv5 {

class JSGlobal : public JSObject {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "global",
      Class::global
    };
    return &cls;
  }
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSGLOBAL_H_
