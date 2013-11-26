#ifndef IV_LV5_RUNTIME_MAP_ITERATOR_H_
#define IV_LV5_RUNTIME_MAP_ITERATOR_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// ES6
// section 23.1.5.2.1 %MapIteratorPrototype%.next()
JSVal MapIteratorNext(const Arguments& args, Error* e);

// ES6
// section 23.1.5.2.2 %MapIteratorPrototype%[@@iterator]()
JSVal MapIteratorIterator(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MAP_ITERATOR_H_
