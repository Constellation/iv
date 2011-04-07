#ifndef _IV_LV5_JSSCRIPT_H_
#define _IV_LV5_JSSCRIPT_H_
#include <gc/gc_cpp.h>
#include <tr1/memory>
#include "icu/source.h"
#include "lv5/jsast.h"
#include "lv5/eval_source.h"
namespace iv {
namespace lv5 {
class Context;

class JSScript : public gc_cleanup {
 public:
  typedef JSScript this_type;
};

class JSInterpreterScript : public JSScript {
 public:
  typedef JSInterpreterScript this_type;
  enum Type {
    kGlobal,
    kEval,
    kFunction
  };

  JSInterpreterScript(const FunctionLiteral* function)
    : function_(function) {
  }

  virtual Type type() const = 0;

  virtual core::UStringPiece SubString(std::size_t start,
                                       std::size_t len) const = 0;

  inline const FunctionLiteral* function() const {
    return function_;
  }

 private:
  const FunctionLiteral* function_;
};

template<typename Source>
class JSEvalScript : public JSInterpreterScript {
 public:
  typedef JSEvalScript<Source> this_type;
  JSEvalScript(const FunctionLiteral* function,
               AstFactory* factory,
               std::tr1::shared_ptr<Source> source)
    : JSInterpreterScript(function),
      factory_(factory),
      source_(source) {
  }

  ~JSEvalScript() {
    // this container has ownership to factory
    delete factory_;
  }

  inline Type type() const {
    return kEval;
  }

  inline std::tr1::shared_ptr<Source> source() const {
    return source_;
  }

  core::UStringPiece SubString(std::size_t start,
                               std::size_t len) const {
    return source_->SubString(start, len);
  }

  static this_type* New(Context* ctx,
                        const FunctionLiteral* function,
                        AstFactory* factory,
                        std::tr1::shared_ptr<Source> source) {
    return new JSEvalScript<Source>(function, factory, source);
  }

 private:
  AstFactory* factory_;
  std::tr1::shared_ptr<Source> source_;
};

class JSGlobalScript : public JSInterpreterScript {
 public:
  typedef JSGlobalScript this_type;
  JSGlobalScript(const FunctionLiteral* function,
                 AstFactory* factory,
                 icu::Source* source)
    : JSInterpreterScript(function),
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

  static this_type* New(Context* ctx,
                        const FunctionLiteral* function,
                        AstFactory* factory,
                        icu::Source* source) {
    return new JSGlobalScript(function, factory, source);
  }

 private:
  icu::Source* source_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSSCRIPT_H_
