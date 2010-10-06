#ifndef _IV_LV5_JSERRORCODE_H_
#define _IV_LV5_JSERRORCODE_H_
namespace iv {
namespace lv5 {
class JSErrorCode {
 public:
  enum Type {
    Normal = 0,
    NativeError,
    EvalError,
    RangeError,
    ReferenceError,
    SyntaxError,
    TypeError,
    URIError
  };
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_ERRORCODE_H_
