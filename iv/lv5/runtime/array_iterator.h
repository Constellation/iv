#ifndef IV_LV5_RUNTIME_ARRAY_ITERATOR_H_
#define IV_LV5_RUNTIME_ARRAY_ITERATOR_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// ES6
// section 22.1.5.2.1 %ArrayIteratorPrototype%.next()
JSVal ArrayIteratorNext(const Arguments& args, Error* e);

// ES6
// section 22.1.5.2.2 %ArrayIteratorPrototype%[@@iterator]()
JSVal ArrayIteratorIterator(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_ARRAY_ITERATOR_H_
