#ifndef IV_LV5_RUNTIME_NUMBER_H_
#define IV_LV5_RUNTIME_NUMBER_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.7.1.1 Number([value])
// section 15.7.2.1 new Number([value])
JSVal NumberConstructor(const Arguments& args, Error* e);

// section 15.7.3.7 Number.isNaN(number)
JSVal NumberIsNaN(const Arguments& args, Error* e);

// section 15.7.3.8 Number.isFinite(number)
JSVal NumberIsFinite(const Arguments& args, Error* e);

// section 15.7.3.9 Number.isInteger(number)
JSVal NumberIsInteger(const Arguments& args, Error* e);

// section 15.7.3.10 Number.toInteger(number)
JSVal NumberToInt(const Arguments& args, Error* e);

// section 15.7.4.2 Number.prototype.toString([radix])
JSVal NumberToString(const Arguments& args, Error* e);

// section 15.7.4.2 Number.prototype.toLocaleString()
// i18n patched version
// section 13.2.1 Number.prototype.toLocaleString([locales [, options]])
JSVal NumberToLocaleString(const Arguments& args, Error* e);

// section 15.7.4.4 Number.prototype.valueOf()
JSVal NumberValueOf(const Arguments& args, Error* e);

// section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
JSVal NumberToFixed(const Arguments& args, Error* e);

// section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
JSVal NumberToExponential(const Arguments& args, Error* e);

// section 15.7.4.7 Number.prototype.toPrecision(precision)
JSVal NumberToPrecision(const Arguments& args, Error* e);

// section 15.7.4.8 Number.prototype.clz()
JSVal NumberCLZ(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_NUMBER_H_
