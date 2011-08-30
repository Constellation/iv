#ifndef IV_LV5_TELEPORTER_FWD_H_
#define IV_LV5_TELEPORTER_FWD_H_
#include "lv5/jsval.h"
#include "lv5/arguments.h"
#include "lv5/error.h"
namespace iv {
namespace lv5 {
namespace teleporter {
// teleporter is lv5 interpreter


class JSCodeFunction;
class Interpreter;

class JSScript;
class Context;

JSVal FunctionConstructor(const Arguments& args, Error* e);
JSVal GlobalEval(const Arguments& args, Error* e);
JSVal DirectCallToEval(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::teleporter
#endif  // IV_LV5_TELEPORTER_FWD_H_
