#ifndef _IV_LV5_JSSCRIPT_H_
#define _IV_LV5_JSSCRIPT_H_
#include <gc/gc_cpp.h>
#include "jsast.h"
#include "icu/source.h"
#include "eval-source.h"
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
  JSScript(const FunctionLiteral* function,
           AstFactory* factory)
    : function_(function),
      factory_(factory) {
  }
  static this_type* NewGlobal(Context* ctx,
                              const FunctionLiteral* function,
                              AstFactory* factory,
                              icu::Source* source);
  static this_type* NewEval(Context* ctx,
                            const FunctionLiteral* function,
                            AstFactory* factory,
                            EvalSource* source);
  static this_type* NewFunction(Context* ctx,
                                const FunctionLiteral* function,
                                AstFactory* factory,
                                EvalSource* source);
  virtual Type type() const = 0;
  virtual core::UStringPiece SubString(std::size_t start,
                                       std::size_t len) const = 0;
  inline const FunctionLiteral* function() const {
    return function_;
  }
  inline AstFactory* factory() const {
    return factory_;
  }
 private:
  const FunctionLiteral* function_;
  AstFactory* factory_;
};

class JSEvalScript : public JSScript {
 public:
  JSEvalScript(const FunctionLiteral* function,
               AstFactory* factory,
               EvalSource* source)
    : JSScript(function, factory),
      source_(source) {
  }
  ~JSEvalScript();
  inline Type type() const {
    return kEval;
  }
  inline EvalSource* source() const {
    return source_;
  }
  core::UStringPiece SubString(std::size_t start,
                               std::size_t len) const {
    return source_->SubString(start, len);
  }
 private:
  EvalSource* source_;
};

class JSGlobalScript : public JSScript {
 public:
  JSGlobalScript(const FunctionLiteral* function,
                 AstFactory* factory,
                 icu::Source* source)
    : JSScript(function, factory),
      source_(source) {
  }
  inline Type type() const {
    return kGlobal;
  }
  inline icu::Source* source() const {
    return source_;
  }
  core::UStringPiece SubString(std::size_t start,
                               std::size_t len) const {
    return source_->SubString(start, len);
  }
 private:
  icu::Source* source_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSSCRIPT_H_
