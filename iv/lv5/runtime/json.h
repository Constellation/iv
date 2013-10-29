#ifndef IV_LV5_RUNTIME_JSON_H_
#define IV_LV5_RUNTIME_JSON_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.12.2 parse(text[, reviver])
JSVal JSONParse(const Arguments& args, Error* e);

// section 15.12.3 stringify(value[, replacer[, space]])
JSVal JSONStringify(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_JSON_H_
