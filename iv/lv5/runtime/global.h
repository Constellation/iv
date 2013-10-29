#ifndef IV_LV5_RUNTIME_GLOBAL_H_
#define IV_LV5_RUNTIME_GLOBAL_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal GlobalParseInt(const Arguments& args, Error* e);

// section 15.1.2.3 parseFloat(string)
JSVal GlobalParseFloat(const Arguments& args, Error* e);

// section 15.1.2.4 isNaN(number)
JSVal GlobalIsNaN(const Arguments& args, Error* e);

// section 15.1.2.5 isFinite(number)
JSVal GlobalIsFinite(const Arguments& args, Error* e);

// section 15.1.3 URI Handling Function Properties
// section 15.1.3.1 decodeURI(encodedURI)
JSVal GlobalDecodeURI(const Arguments& args, Error* e);

// section 15.1.3.2 decodeURIComponent(encodedURIComponent)
JSVal GlobalDecodeURIComponent(const Arguments& args, Error* e);

// section 15.1.3.3 encodeURI(uri)
JSVal GlobalEncodeURI(const Arguments& args, Error* e);

// section 15.1.3.4 encodeURIComponent(uriComponent)
JSVal GlobalEncodeURIComponent(const Arguments& args, Error* e);

// section B.2.1 escape(string)
// this method is deprecated.
JSVal GlobalEscape(const Arguments& args, Error* e);

// section B.2.2 unescape(string)
// this method is deprecated.
JSVal GlobalUnescape(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_GLOBAL_H_
