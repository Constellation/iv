#ifndef IV_LV5_RUNTIME_WEAK_MAP_H_
#define IV_LV5_RUNTIME_WEAK_MAP_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// 15.15.2.1 WeakMap(iterable = [])
// 15.15.3.1 new WeakMap (iterable = [])
JSVal WeakMapConstructor(const Arguments& args, Error* e);

// 15.15.5.2 WeakMap.prototype.clear()
JSVal WeakMapClear(const Arguments& args, Error* e);

// 15.15.5.3 WeakMap.prototype.delete(key)
JSVal WeakMapDelete(const Arguments& args, Error* e);

// 15.15.5.4 WeakMap.prototype.get(key)
JSVal WeakMapGet(const Arguments& args, Error* e);

// 15.15.5.5 WeakMap.prototype.has(key)
JSVal WeakMapHas(const Arguments& args, Error* e);

// 15.15.5.6 WeakMap.prototype.set(key, value)
JSVal WeakMapSet(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_WEAK_MAP_H_
