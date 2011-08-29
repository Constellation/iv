#ifndef _IV_LV5_JSGLOBAL_H_
#define _IV_LV5_JSGLOBAL_H_
#include "notfound.h"
#include "lv5/error_check.h"
#include "lv5/map.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/class.h"
#include "lv5/arguments.h"
#include "lv5/object_utils.h"
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

  explicit JSGlobal(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)) {
    assert(map()->GetSlotsSize() == 0);
  }
};

void Map::GetOwnPropertyNames(const JSObject* obj,
                              Context* ctx,
                              std::vector<Symbol>* vec,
                              JSObject::EnumerationMode mode) {
  if (AllocateTableIfNeeded()) {
    for (TargetTable::const_iterator it = table_->begin(),
         last = table_->end(); it != last; ++it) {
      if ((mode == JSObject::kIncludeNotEnumerable ||
           obj->GetSlot(it->second).IsEnumerable()) &&
          (std::find(vec->begin(), vec->end(), it->first) == vec->end())) {
        vec->push_back(it->first);
      }
    }
  }
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSGLOBAL_H_
