#ifndef IV_LV5_RUNTIME_MAP_H_
#define IV_LV5_RUNTIME_MAP_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.14.2.1 Map(iterable = [])
// section 15.14.3.1 new Map (iterable = [])
JSVal MapConstructor(const Arguments& args, Error* e);

// section 15.14.5.2 Map.prototype.clear()
JSVal MapClear(const Arguments& args, Error* e);

// section 15.14.5.3 Map.prototype.delete(key)
JSVal MapDelete(const Arguments& args, Error* e);

// 15.14.5.4 Map.prototype.forEach(callbackfn, thisArg = undefined)
JSVal MapForEach(const Arguments& args, Error* e);

// section 15.14.5.5 Map.prototype.get(key)
JSVal MapGet(const Arguments& args, Error* e);

// section 15.14.5.6 Map.prototype.has(key)
JSVal MapHas(const Arguments& args, Error* e);

// TODO(Constellation) iv / lv5 doesn't have iterator system
// 15.14.5.7 Map.prototype.items()
// 15.14.5.8 Map.prototype.keys()

// 15.14.5.9 Map.prototype.set(key, value)
JSVal MapSet(const Arguments& args, Error* e);

// 15.14.5.9 get Map.prototype.size
JSVal MapSize(const Arguments& args, Error* e);

// TODO(Constellation) iv / lv5 doesn't have iterator system
// 15.14.5.10 Map.prototype.values()
// 15.14.5.11 Map.prototype.@@iterator

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MAP_H_
