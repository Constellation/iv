#ifndef IV_LV5_JSNAME_H_
#define IV_LV5_JSNAME_H_
#include <iv/lv5/gc_template.h>
namespace iv {
namespace lv5 {

class JSName : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(Name)
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSNAME_H_
