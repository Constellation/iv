#ifndef IV_LV5_RUNTIME_OBJECT_H_
#define IV_LV5_RUNTIME_OBJECT_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.2.1.1 Object([value])
// section 15.2.2.1 new Object([value])
JSVal ObjectConstructor(const Arguments& args, Error* e);

// section 15.2.3.2 Object.getPrototypeOf(O)
JSVal ObjectGetPrototypeOf(const Arguments& args, Error* e);

// section 15.2.3.3 Object.getOwnPropertyDescriptor(O, P)
JSVal ObjectGetOwnPropertyDescriptor(const Arguments& args, Error* e);

// section 15.2.3.4 Object.getOwnPropertyNames(O)
JSVal ObjectGetOwnPropertyNames(const Arguments& args, Error* e);

// section 15.2.3.5 Object.create(O[, Properties])
JSVal ObjectCreate(const Arguments& args, Error* e);

// section 15.2.3.6 Object.defineProperty(O, P, Attributes)
JSVal ObjectDefineProperty(const Arguments& args, Error* e);

// section 15.2.3.7 Object.defineProperties(O, Properties)
JSVal ObjectDefineProperties(const Arguments& args, Error* e);

// section 15.2.3.8 Object.seal(O)
JSVal ObjectSeal(const Arguments& args, Error* e);

// section 15.2.3.9 Object.freeze(O)
JSVal ObjectFreeze(const Arguments& args, Error* e);

// section 15.2.3.10 Object.preventExtensions(O)
JSVal ObjectPreventExtensions(const Arguments& args, Error* e);

// section 15.2.3.11 Object.isSealed(O)
JSVal ObjectIsSealed(const Arguments& args, Error* e);

// section 15.2.3.12 Object.isFrozen(O)
JSVal ObjectIsFrozen(const Arguments& args, Error* e);

// section 15.2.3.13 Object.isExtensible(O)
JSVal ObjectIsExtensible(const Arguments& args, Error* e);

// section 15.2.3.14 Object.keys(O)
JSVal ObjectKeys(const Arguments& args, Error* e);

// ES.next Object.is(x, y)
// http://wiki.ecmascript.org/doku.php?id=harmony:egal
JSVal ObjectIs(const Arguments& args, Error* e);

// section 15.2.4.2 Object.prototype.toString()
JSVal ObjectToString(const Arguments& args, Error* e);

// section 15.2.4.3 Object.prototype.toLocaleString()
JSVal ObjectToLocaleString(const Arguments& args, Error* e);

// section 15.2.4.4 Object.prototype.valueOf()
JSVal ObjectValueOf(const Arguments& args, Error* e);

// section 15.2.4.5 Object.prototype.hasOwnProperty(V)
JSVal ObjectHasOwnProperty(const Arguments& args, Error* e);

// section 15.2.4.6 Object.prototype.isPrototypeOf(V)
JSVal ObjectIsPrototypeOf(const Arguments& args, Error* e);

// section 15.2.4.7 Object.prototype.propertyIsEnumerable(V)
JSVal ObjectPropertyIsEnumerable(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_OBJECT_H_
