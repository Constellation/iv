#ifndef _IV_LV5_JSARGUMENTS_H_
#define _IV_LV5_JSARGUMENTS_H_
#include "ast.h"
#include "lv5/symbol.h"
#include "lv5/property.h"
#include "lv5/arguments.h"
#include "lv5/gc_template.h"
namespace iv {
namespace lv5 {

class Context;
class JSObject;
class Error;
class JSDeclEnv;
class AstFactory;
class JSCodeFunction;

class JSArguments : public JSObject {
 public:
  typedef core::SpaceVector<AstFactory, Identifier*>::type Identifiers;
  typedef GCHashMap<Symbol, Symbol>::type Index2Param;
  JSArguments(Context* ctx, JSDeclEnv* env)
    : env_(env),
      map_() { }

  static JSArguments* New(Context* ctx,
                          JSCodeFunction* code,
                          const Identifiers& names,
                          const Arguments& args,
                          JSDeclEnv* env,
                          bool strict);
  JSVal Get(Context* context,
            Symbol name, Error* error);
  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;
  bool DefineOwnProperty(Context* ctx,
                         Symbol name,
                         const PropertyDescriptor& desc,
                         bool th,
                         Error* error);
  bool Delete(Context* ctx, Symbol name, bool th, Error* error);
 private:
  JSDeclEnv* env_;
  Index2Param map_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARGUMENTS_H_
