#ifndef IV_LV5_RUNTIME_DATE_H_
#define IV_LV5_RUNTIME_DATE_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.9.2.1
//   Date([year[, month[, date[, hours[, minutes[, seconds[, ms]]]])
// section 15.9.3.1
//   new Date(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
JSVal DateConstructor(const Arguments& args, Error* e);

// section 15.9.4.2 Date.parse(string)
JSVal DateParse(const Arguments& args, Error* e);

// section 15.9.4.3
// Date.UTC(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
JSVal DateUTC(const Arguments& args, Error* e);

// section 15.9.4.4 Date.now()
JSVal DateNow(const Arguments& args, Error* e);

// section 15.9.5.2 Date.prototype.toString()
JSVal DateToString(const Arguments& args, Error* e);

// section 15.9.5.3 Date.prototype.toDateString()
JSVal DateToDateString(const Arguments& args, Error* e);

// section 15.9.5.4 Date.prototype.toTimeString()
JSVal DateToTimeString(const Arguments& args, Error* e);

// section 15.9.5.5 Date.prototype.toLocaleString()
JSVal DateToLocaleString(const Arguments& args, Error* e);

// section 15.9.5.6 Date.prototype.toLocaleDateString()
JSVal DateToLocaleDateString(const Arguments& args, Error* e);

// section 15.9.5.7 Date.prototype.toLocaleTimeString()
JSVal DateToLocaleTimeString(const Arguments& args, Error* e);

// section 15.9.5.8 Date.prototype.valueOf()
JSVal DateValueOf(const Arguments& args, Error* e);

// section 15.9.5.9 Date.prototype.getTime()
JSVal DateGetTime(const Arguments& args, Error* e);

// section 15.9.5.10 Date.prototype.getFullYear()
JSVal DateGetFullYear(const Arguments& args, Error* e);

// section 15.9.5.11 Date.prototype.getUTCFullYear()
JSVal DateGetUTCFullYear(const Arguments& args, Error* e);

// section 15.9.5.12 Date.prototype.getMonth()
JSVal DateGetMonth(const Arguments& args, Error* e);

// section 15.9.5.13 Date.prototype.getUTCMonth()
JSVal DateGetUTCMonth(const Arguments& args, Error* e);

// section 15.9.5.14 Date.prototype.getDate()
JSVal DateGetDate(const Arguments& args, Error* e);

// section 15.9.5.15 Date.prototype.getUTCDate()
JSVal DateGetUTCDate(const Arguments& args, Error* e);

// section 15.9.5.16 Date.prototype.getDay()
JSVal DateGetDay(const Arguments& args, Error* e);

// section 15.9.5.17 Date.prototype.getUTCDay()
JSVal DateGetUTCDay(const Arguments& args, Error* e);

// section 15.9.5.18 Date.prototype.getHours()
JSVal DateGetHours(const Arguments& args, Error* e);

// section 15.9.5.19 Date.prototype.getUTCHours()
JSVal DateGetUTCHours(const Arguments& args, Error* e);

// section 15.9.5.20 Date.prototype.getMinutes()
JSVal DateGetMinutes(const Arguments& args, Error* e);

// section 15.9.5.21 Date.prototype.getUTCMinutes()
JSVal DateGetUTCMinutes(const Arguments& args, Error* e);

// section 15.9.5.22 Date.prototype.getSeconds()
JSVal DateGetSeconds(const Arguments& args, Error* e);

// section 15.9.5.23 Date.prototype.getUTCSeconds()
JSVal DateGetUTCSeconds(const Arguments& args, Error* e);

// section 15.9.5.24 Date.prototype.getMilliseconds()
JSVal DateGetMilliseconds(const Arguments& args, Error* e);

// section 15.9.5.25 Date.prototype.getUTCMilliseconds()
JSVal DateGetUTCMilliseconds(const Arguments& args, Error* e);

// section 15.9.5.26 Date.prototype.getTimezoneOffset()
JSVal DateGetTimezoneOffset(const Arguments& args, Error* e);

// section 15.9.5.27 Date.prototype.setTime(time)
JSVal DateSetTime(const Arguments& args, Error* e);

// section 15.9.5.28 Date.prototype.setMilliseconds(ms)
JSVal DateSetMilliseconds(const Arguments& args, Error* e);

// section 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
JSVal DateSetUTCMilliseconds(const Arguments& args, Error* e);

// section 15.9.5.30 Date.prototype.setSeconds(sec[, ms])
JSVal DateSetSeconds(const Arguments& args, Error* e);

// section 15.9.5.31 Date.prototype.setUTCSeconds(sec[, ms])
JSVal DateSetUTCSeconds(const Arguments& args, Error* e);

// section 15.9.5.32 Date.prototype.setMinutes(min[, sec[, ms]])
JSVal DateSetMinutes(const Arguments& args, Error* e);

// section 15.9.5.33 Date.prototype.setUTCMinutes(min[, sec[, ms]])
JSVal DateSetUTCMinutes(const Arguments& args, Error* e);

// section 15.9.5.34 Date.prototype.setHours(hour[, min[, sec[, ms]])
JSVal DateSetHours(const Arguments& args, Error* e);

// section 15.9.5.35 Date.prototype.setUTCHours(hour[, min[, sec[, ms]])
JSVal DateSetUTCHours(const Arguments& args, Error* e);

// section 15.9.5.36 Date.prototype.setDate(date)
JSVal DateSetDate(const Arguments& args, Error* e);

// section 15.9.5.37 Date.prototype.setUTCDate(date)
JSVal DateSetUTCDate(const Arguments& args, Error* e);

// section 15.9.5.38 Date.prototype.setMonth(month[, date])
JSVal DateSetMonth(const Arguments& args, Error* e);

// section 15.9.5.39 Date.prototype.setUTCMonth(month[, date])
JSVal DateSetUTCMonth(const Arguments& args, Error* e);

// section 15.9.5.40 Date.prototype.setFullYear(year[, month[, date]])
JSVal DateSetFullYear(const Arguments& args, Error* e);

// section 15.9.5.41 Date.prototype.setUTCFullYear(year[, month[, date]])
JSVal DateSetUTCFullYear(const Arguments& args, Error* e);

// section 15.9.5.42 Date.prototype.toUTCString()
JSVal DateToUTCString(const Arguments& args, Error* e);

// section 15.9.5.43 Date.prototype.toISOString()
JSVal DateToISOString(const Arguments& args, Error* e);

// section 15.9.5.44 Date.prototype.toJSON()
JSVal DateToJSON(const Arguments& args, Error* e);

// section B.2.4 Date.prototype.getYear()
// this method is deprecated.
JSVal DateGetYear(const Arguments& args, Error* e);

// section B.2.5 Date.prototype.setYear(year)
// this method is deprecated.
JSVal DateSetYear(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_STRING_H_
