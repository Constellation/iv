#ifndef IV_LV5_RUNTIME_STRING_H_
#define IV_LV5_RUNTIME_STRING_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.5.1
JSVal StringConstructor(const Arguments& args, Error* e);

// section 15.5.3.2 String.fromCharCode([char0 [, char1[, ...]]])
JSVal StringFromCharCode(const Arguments& args, Error* e);

// ES6
// section 15.5.3.3 String.fromCodePoint(...codePoints)
JSVal StringFromCodePoint(const Arguments& args, Error* e);

// ES6
// section 15.5.3.4 String.raw(callSite, ...substitutions)
JSVal StringRaw(const Arguments& args, Error* e);

// section 15.5.4.2 String.prototype.toString()
JSVal StringToString(const Arguments& args, Error* e);

// section 15.5.4.3 String.prototype.valueOf()
JSVal StringValueOf(const Arguments& args, Error* e);

// section 15.5.4.4 String.prototype.charAt(pos)
JSVal StringCharAt(const Arguments& args, Error* e);

// section 15.5.4.5 String.prototype.charCodeAt(pos)
JSVal StringCharCodeAt(const Arguments& args, Error* e);

// ES6
// section 15.5.4.5 String.prototype.codePointAt(pos)
JSVal StringCodePointAt(const Arguments& args, Error* e);

// section 15.5.4.6 String.prototype.concat([string1[, string2[, ...]]])
JSVal StringConcat(const Arguments& args, Error* e);

// section 15.5.4.7 String.prototype.indexOf(searchString, position)
JSVal StringIndexOf(const Arguments& args, Error* e);

// section 15.5.4.8 String.prototype.lastIndexOf(searchString, position)
JSVal StringLastIndexOf(const Arguments& args, Error* e);

// section 15.5.4.9 String.prototype.localeCompare(that)
JSVal StringLocaleCompare(const Arguments& args, Error* e);

// section 15.5.4.10 String.prototype.match(regexp)
JSVal StringMatch(const Arguments& args, Error* e);

// section 15.5.4.11 String.prototype.replace(searchValue, replaceValue)
JSVal StringReplace(const Arguments& args, Error* e);

// section 15.5.4.12 String.prototype.search(regexp)
JSVal StringSearch(const Arguments& args, Error* e);

// section 15.5.4.13 String.prototype.slice(start, end)
JSVal StringSlice(const Arguments& args, Error* e);

// section 15.5.4.14 String.prototype.split(separator, limit)
JSVal StringSplit(const Arguments& args, Error* e);

// section 15.5.4.15 String.prototype.substring(start, end)
JSVal StringSubstring(const Arguments& args, Error* e);

// section 15.5.4.16 String.prototype.toLowerCase()
JSVal StringToLowerCase(const Arguments& args, Error* e);

// section 15.5.4.17 String.prototype.toLocaleLowerCase()
JSVal StringToLocaleLowerCase(const Arguments& args, Error* e);

// section 15.5.4.18 String.prototype.toUpperCase()
JSVal StringToUpperCase(const Arguments& args, Error* e);

// section 15.5.4.19 String.prototype.toLocaleUpperCase()
JSVal StringToLocaleUpperCase(const Arguments& args, Error* e);

// section 15.5.4.20 String.prototype.trim()
JSVal StringTrim(const Arguments& args, Error* e);

// section 15.5.4.21 String.prototype.repeat(count)
JSVal StringRepeat(const Arguments& args, Error* e);

// section 15.5.4.22 String.prototype.startsWith(searchString, [position])
JSVal StringStartsWith(const Arguments& args, Error* e);

// section 15.5.4.23 String.prototype.endsWith(searchString, [endPosition])
JSVal StringEndsWith(const Arguments& args, Error* e);

// section 15.5.4.24 String.prototype.contains(searchString, [position])
JSVal StringContains(const Arguments& args, Error* e);

// section 15.5.4.25 String.prototype.reverse()
JSVal StringReverse(const Arguments& args, Error* e);

// section B.2.3 String.prototype.substr(start, length)
// this method is deprecated.
JSVal StringSubstr(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_STRING_H_
