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

// section 15.16.5.2 Set.prototype.add(value)
JSVal SetAdd(const Arguments& args, Error* e);

// section 15.16.5.3 Set.prototype.clear()
JSVal SetClear(const Arguments& args, Error* e);

// section 15.16.5.4 Set.prototype.delete(value)
JSVal SetDelete(const Arguments& args, Error* e);

// 15.16.5.5 Set.prototype.forEach(callbackfn, thisArg = undefined)
JSVal SetForEach(const Arguments& args, Error* e);

// section 15.16.5.6 Set.prototype.has(value)
JSVal SetHas(const Arguments& args, Error* e);

// 15.16.5.7 get Set.prototype.size
JSVal SetSize(const Arguments& args, Error* e);

// TODO(Constellation) iv / lv5 doesn't have iterator system
// 15.16.5.8 Set.prototype.values()
// 15.16.5.9 Set.prototype.@@iterator

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_SET_H_
