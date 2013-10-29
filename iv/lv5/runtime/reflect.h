#ifndef IV_LV5_RUNTIME_REFLECT_H_
#define IV_LV5_RUNTIME_REFLECT_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// 15.17.1.1 Reflect.getPrototypeOf(target)
JSVal ReflectGetPrototypeOf(const Arguments& args, Error* e);

// 15.17.1.2 Reflect.setPrototypeOf(target, proto)
JSVal ReflectSetPrototypeOf(const Arguments& args, Error* e);

// 15.17.1.3 Reflect.isExtensible(target)
JSVal ReflectIsExtensible(const Arguments& args, Error* e);

// 15.17.1.4 Reflect.preventExtensions(target)
JSVal ReflectPreventExtensions(const Arguments& args, Error* e);

// 15.17.1.5 Reflect.hasOwn(target, propertyKey)
JSVal ReflectHasOwn(const Arguments& args, Error* e);

// 15.17.1.6 Reflect.getOwnPropertyDescriptor(target, propertyKey)
JSVal ReflectGetOwnPropertyDescriptor(const Arguments& args, Error* e);

// 15.17.1.7 Reflect.get(target, propertyKey, receiver=target)
// JSVal ReflectGet(const Arguments& args, Error* e);

// 15.17.1.8 Reflect.set(target, propertyKey, V, receiver=target)
// JSVal ReflectSet(const Arguments& args, Error* e);

// 15.17.1.9 Reflect.deleteProperty(target, propertyKey)
JSVal ReflectDeleteProperty(const Arguments& args, Error* e);

// 15.17.1.10 Reflect.defineProperty(target, propertyKey, attributes)
JSVal ReflectDefineProperty(const Arguments& args, Error* e);

// 15.17.1.11 Reflect.enumerate(target)
// JSVal ReflectEnumerate(const Arguments& args, Error* e);

// 15.17.1.12 Reflect.keys(target)
JSVal ReflectKeys(const Arguments& args, Error* e);

// 15.17.1.13 Reflect.getOwnPropertyNames(target)
JSVal ReflectGetOwnPropertyNames(const Arguments& args, Error* e);

// 15.17.1.14 Reflect.freeze(target)
JSVal ReflectFreeze(const Arguments& args, Error* e);

// 15.17.1.14 Reflect.seal(target)
JSVal ReflectSeal(const Arguments& args, Error* e);

// 15.17.1.16 Reflect.isFrozen(target)
JSVal ReflectIsFrozen(const Arguments& args, Error* e);

// 15.17.1.17 Reflect.isSealed(target)
JSVal ReflectIsSealed(const Arguments& args, Error* e);

// 15.17.1.18 Reflect.has(target, propertyKey)
JSVal ReflectHas(const Arguments& args, Error* e);

// 15.17.2.2 Reflect.instanceOf(target, O)
JSVal ReflectInstanceOf(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_REFLECT_H_
