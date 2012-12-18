#ifndef IV_LV5_JSJSON_H_
#define IV_LV5_JSJSON_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSJSON : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSJSON, JSON)
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSJSON_H_
