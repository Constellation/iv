#ifndef IV_LV5_RUNTIME_ARRAY_H_
#define IV_LV5_RUNTIME_ARRAY_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.4.1.1 Array([item0 [, item1 [, ...]]])
// section 15.4.2.1 new Array([item0 [, item1 [, ...]]])
// section 15.4.2.2 new Array(len)
JSVal ArrayConstructor(const Arguments& args, Error* e);

// section 15.4.3.2 Array.isArray(arg)
JSVal ArrayIsArray(const Arguments& args, Error* e);

// strawman / ES.next Array extras
//
//   Array.from(arg)
//   Array.of()
//
// These functions are experimental.
// They may be changed without any notice.
//
// http://wiki.ecmascript.org/doku.php?id=strawman:array_extras

// section 15.4.3.3 Array.of(...items)
JSVal ArrayOf(const Arguments& args, Error* e);

// section 15.4.3.4 Array.from(arrayLike)
JSVal ArrayFrom(const Arguments& args, Error* e);

// section 15.4.4.2 Array.prototype.toString()
JSVal ArrayToString(const Arguments& args, Error* e);

// section 15.4.4.3 Array.prototype.toLocaleString()
JSVal ArrayToLocaleString(const Arguments& args, Error* e);

// section 15.4.4.4 Array.prototype.concat([item1[, item2[, ...]]])
JSVal ArrayConcat(const Arguments& args, Error* e);

// section 15.4.4.5 Array.prototype.join(separator)
JSVal ArrayJoin(const Arguments& args, Error* e);

// section 15.4.4.6 Array.prototype.pop()
JSVal ArrayPop(const Arguments& args, Error* e);

// section 15.4.4.7 Array.prototype.push([item1[, item2[, ...]]])
JSVal ArrayPush(const Arguments& args, Error* e);

// section 15.4.4.8 Array.prototype.reverse()
JSVal ArrayReverse(const Arguments& args, Error* e);

// section 15.4.4.9 Array.prototype.shift()
JSVal ArrayShift(const Arguments& args, Error* e);

// section 15.4.4.10 Array.prototype.slice(start, end)
JSVal ArraySlice(const Arguments& args, Error* e);

// section 15.4.4.11 Array.prototype.sort(comparefn)
// non recursive quick sort
JSVal ArraySort(const Arguments& args, Error* e);

// section 15.4.4.12
// Array.prototype.splice(start, deleteCount[, item1[, item2[, ...]]])
JSVal ArraySplice(const Arguments& args, Error* e);

// section 15.4.4.13 Array.prototype.unshift([item1[, item2[, ...]]])
JSVal ArrayUnshift(const Arguments& args, Error* e);

// section 15.4.4.14 Array.prototype.indexOf(searchElement[, fromIndex])
JSVal ArrayIndexOf(const Arguments& args, Error* e);

// section 15.4.4.15 Array.prototype.lastIndexOf(searchElement[, fromIndex])
JSVal ArrayLastIndexOf(const Arguments& args, Error* e);

// section 15.4.4.16 Array.prototype.every(callbackfn[, thisArg])
JSVal ArrayEvery(const Arguments& args, Error* e);

// section 15.4.4.17 Array.prototype.some(callbackfn[, thisArg])
JSVal ArraySome(const Arguments& args, Error* e);

// section 15.4.4.18 Array.prototype.forEach(callbackfn[, thisArg])
JSVal ArrayForEach(const Arguments& args, Error* e);

// section 15.4.4.19 Array.prototype.map(callbackfn[, thisArg])
JSVal ArrayMap(const Arguments& args, Error* e);

// section 15.4.4.20 Array.prototype.filter(callbackfn[, thisArg])
JSVal ArrayFilter(const Arguments& args, Error* e);

// section 15.4.4.21 Array.prototype.reduce(callbackfn[, initialValue])
JSVal ArrayReduce(const Arguments& args, Error* e);

// section 15.4.4.22 Array.prototype.reduceRight(callbackfn[, initialValue])
JSVal ArrayReduceRight(const Arguments& args, Error* e);

// ES6
// section 22.1.3.4 Array.prototype.entries()
JSVal ArrayEntries(const Arguments& args, Error* e);

// ES6
// section 22.1.3.13 Array.prototype.keys()
JSVal ArrayKeys(const Arguments& args, Error* e);

// ES6
// section 22.1.3.29 Array.prototype.values()
JSVal ArrayValues(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_ARRAY_H_
