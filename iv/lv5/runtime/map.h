#ifndef IV_LV5_RUNTIME_MAP_H_
#define IV_LV5_RUNTIME_MAP_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// TODO(Yusuke Suzuki): Fix this signature.
// 15.14.2.1 Map(iterable = [])
// 15.14.3.1 new Map (iterable = [])
JSVal MapConstructor(const Arguments& args, Error* e);

// 23.1.3.1 Map.prototype.clear()
JSVal MapClear(const Arguments& args, Error* e);

// 23.1.3.3 Map.prototype.delete(key)
JSVal MapDelete(const Arguments& args, Error* e);

// 23.1.3.4 Map.prototype.entries()
JSVal MapEntries(const Arguments& args, Error* e);

// 23.1.3.5 Map.prototype.forEach(callbackfn, thisArg = undefined)
JSVal MapForEach(const Arguments& args, Error* e);

// 23.1.3.6 Map.prototype.get(key)
JSVal MapGet(const Arguments& args, Error* e);

// 23.1.3.7 Map.prototype.has(key)
JSVal MapHas(const Arguments& args, Error* e);

// 23.1.3.8 Map.prototype.keys()
JSVal MapKeys(const Arguments& args, Error* e);

// 23.1.3.9 Map.prototype.set(key, value)
JSVal MapSet(const Arguments& args, Error* e);

// 23.1.3.10 get Map.prototype.size
JSVal MapSize(const Arguments& args, Error* e);

// 23.1.3.11 Map.prototype.values()
JSVal MapValues(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MAP_H_
