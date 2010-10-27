#ifndef _IV_LV5_JSARGUMENTS_H_
#define _IV_LV5_JSARGUMENTS_H_
#include "ast.h"
#include "symbol.h"
#include "property.h"
#include "arguments.h"
#include "gc-template.h"
#include "jsfunction.h"
namespace iv {
namespace lv5 {

class Context;
class JSObject;
class Error;
class JSDeclEnv;
class JSArguments : public JSObject {
 public:
  typedef core::AstNode::Identifiers Identifiers;
  typedef GCHashMap<Symbol, Symbol>::type Index2Param;
  JSArguments(Context* ctx, JSDeclEnv* env)
    : env_(env),
      map_() { }
  static JSArguments* New(Context* ctx,
                          const JSCodeFunction& code,
                          const Identifiers& names,
                          const Arguments& args,
                          JSDeclEnv* env,
                          bool strict);
  JSVal Get(Context* context,
            Symbol name, Error* error);
  PropertyDescriptor GetOwnProperty(Symbol name) const;
  bool DefineOwnProperty(Context* ctx,
                         Symbol name,
                         const PropertyDescriptor& desc,
                         bool th,
                         Error* error);
  bool Delete(Symbol name, bool th, Error* error);
 private:
  JSDeclEnv* env_;
  Index2Param map_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARGUMENTS_H_
