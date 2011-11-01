#ifndef IV_LV5_JSGLOBAL_H_
#define IV_LV5_JSGLOBAL_H_
#include <iv/notfound.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/map.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/class.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/object_utils.h>
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

  static JSGlobal* New(Context* ctx) { return new JSGlobal(ctx); }

 private:
  explicit JSGlobal(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)) {
    assert(map()->GetSlotsSize() == 0);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSGLOBAL_H_
