#ifndef IV_LV5_JSJSON_H_
#define IV_LV5_JSJSON_H_
#include <iv/lv5/jsobject.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSJSON : public JSObject {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "JSON",
      Class::JSON
    };
    return &cls;
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSJSON_H_
