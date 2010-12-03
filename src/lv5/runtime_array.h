#ifndef _IV_LV5_RUNTIME_ARRAY_H_
#define _IV_LV5_RUNTIME_ARRAY_H_
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsarray.h"

namespace iv {
namespace lv5 {
namespace runtime {

// section 15.4.1.1 Array([item0 [, item1 [, ...]]])
// section 15.4.2.1 new Array([item0 [, item1 [, ...]]])
// section 15.4.2.2 new Array(len)
inline JSVal ArrayConstructor(const Arguments& args, Error* error) {
// TODO(Constellation) this is mock function
  return JSArray::New(args.ctx());
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_ARRAY_H_
