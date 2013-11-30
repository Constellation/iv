#ifndef IV_LV5_RUNTIME_SET_H_
#define IV_LV5_RUNTIME_SET_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.16.2.1 Set(iterable = undefined)
// section 15.16.3.1 new Set (iterable = undefined)
JSVal SetConstructor(const Arguments& args, Error* e);

// 23.2.3.1 Set.prototype.add(value)
JSVal SetAdd(const Arguments& args, Error* e);

// 23.2.3.2 Set.prototype.clear()
JSVal SetClear(const Arguments& args, Error* e);

// 23.2.3.4 Set.prototype.delete(value)
JSVal SetDelete(const Arguments& args, Error* e);

// 23.2.3.5 Set.prototype.entries()
JSVal SetEntries(const Arguments& args, Error* e);

// 23.2.3.6 Set.prototype.forEach(callbackfn, thisArg = undefined)
JSVal SetForEach(const Arguments& args, Error* e);

// 23.2.3.7 Set.prototype.has(value)
JSVal SetHas(const Arguments& args, Error* e);

// 23.2.3.8 Set.prototype.keys()
JSVal SetKeys(const Arguments& args, Error* e);

// 23.2.3.9 get Set.prototype.size
JSVal SetSize(const Arguments& args, Error* e);

// 23.2.3.10 Set.prototype.values()
JSVal SetValues(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_SET_H_
