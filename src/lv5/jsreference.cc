#include "jsreference.h"

namespace iv {
namespace lv5 {


JSReference* JSReference::New(Context* ctx, JSVal base,
                              Symbol name, bool is_strict) {
  return new JSReference(base, name, is_strict);
}


} }  // namespace iv::lv5
