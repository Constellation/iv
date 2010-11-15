#ifndef _IV_LV5_JSSCRIPT_H_
#define _IV_LV5_JSSCRIPT_H_
#include <gc/gc_cpp.h>
#include "source.h"
#include "jsast.h"
namespace iv {
namespace lv5 {
class Context;

class JSScript : public gc_cleanup {
 public:
  typedef JSScript this_type;
  enum Type {
    kGlobal,
    kEval,
    kFunction
  };
  JSScript(Type type,
           const FunctionLiteral* function,
           AstFactory* factory,
           core::BasicSource* source)
    : type_(type),
      function_(function),
      factory_(factory),
      source_(source) {
  }
  ~JSScript();
  static this_type* NewGlobal(Context* ctx,
                              const FunctionLiteral* function,
                              AstFactory* factory,
                              core::BasicSource* source);
  static this_type* NewEval(Context* ctx,
                            const FunctionLiteral* function,
                            AstFactory* factory,
                            core::BasicSource* source);
  static this_type* NewFunction(Context* ctx,
                                const FunctionLiteral* function,
                                AstFactory* factory,
                                core::BasicSource* source);
  inline Type type() const {
    return type_;
  }
  inline const FunctionLiteral* function() const {
    return function_;
  }
  inline AstFactory* factory() const {
    return factory_;
  }
  inline core::BasicSource* source() const {
    return source_;
  }
 private:
  Type type_;
  const FunctionLiteral* function_;
  AstFactory* factory_;
  core::BasicSource* source_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSSCRIPT_H_
