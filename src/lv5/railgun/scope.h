#ifndef _IV_LV5_RAILGUN_SCOPE_H_
#define _IV_LV5_RAILGUN_SCOPE_H_
#include <algorithm>
#include "detail/memory.h"
#include "detail/unordered_map.h"
#include "lv5/specialized_ast.h"
#include "lv5/symbol.h"
#include "lv5/context_utils.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/code.h"
namespace iv {
namespace lv5 {
namespace railgun {

class VariableScope : private core::Noncopyable<VariableScope> {
 public:
  enum Type {
    STACK = 0,
    HEAP,
    GLOBAL,
    LOOKUP
  };

  virtual ~VariableScope() { }

  std::shared_ptr<VariableScope> upper() const {
    return upper_;
  }

  void Lookup(Symbol sym, std::size_t target) {
    // first, so this is stack
    LookupImpl(sym, target, (InWith())? LOOKUP : STACK);
  }

  virtual bool InWith() const {
    return false;
  }

  static Type TypeUpgrade(Type now, Type next) {
    return std::max<Type>(now, next);
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type) = 0;

  virtual void MakeUpperOfEval() { }

  void RecordEval() {
    MakeUpperOfEval();
    VariableScope* scope = GetRawUpper();
    while (scope) {
      scope->MakeUpperOfEval();
      scope = scope->GetRawUpper();
    }
  }

 protected:
  explicit VariableScope(std::shared_ptr<VariableScope> upper)
    : upper_(upper) {
  }

  VariableScope* GetRawUpper() const {
    return upper_.get();
  }

  std::shared_ptr<VariableScope> upper_;
};

class WithScope : public VariableScope {
 public:
  explicit WithScope(std::shared_ptr<VariableScope> upper)
    : VariableScope(upper) {
    assert(upper);
  }

  bool InWith() {
    return true;
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type) {
    upper_->LookupImpl(sym, target, TypeUpgrade(type, LOOKUP));
  }
};

class CatchScope : public VariableScope {
 public:
  CatchScope(std::shared_ptr<VariableScope> upper, Symbol sym)
    : VariableScope(upper),
      target_(sym) {
    assert(upper);
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type) {
    if (sym != target_) {
      upper_->LookupImpl(sym, target, TypeUpgrade(type, HEAP));
    }
  }

 private:
  Symbol target_;
};

class FunctionScope : public VariableScope {
 public:

  typedef std::vector<std::pair<Symbol, std::size_t> > Labels;
  typedef std::unordered_map<Symbol, Type> Variables;

  FunctionScope(std::shared_ptr<VariableScope> upper, Code::Data* data)
    : VariableScope(upper),
      map_(),
      labels_(),
      code_(NULL),
      data_(data),
      upper_of_eval_(false),
      eval_top_scope_(false) {
  }

  FunctionScope(std::shared_ptr<VariableScope> upper,
                Context* ctx,
                Code* code,
                Code::Data* data,
                const Scope& scope)
    : VariableScope(upper),
      map_(),
      labels_(),
      code_(code),
      data_(data),
      upper_of_eval_(false),
      eval_top_scope_(code->code_type() == Code::EVAL) {
    // is global or not
    const Type default_type =
        (IsTop()) ? (eval_top_scope_) ? LOOKUP : GLOBAL : STACK;
    if (!IsTop()) {
      map_[symbol::arguments] = HEAP;
    }

    // params
    for (Code::Names::const_iterator it = code->params().begin(),
         last = code->params().end(); it != last; ++it) {
      // TODO(Constellation) optimize it
      // map_[*it] = default_type;
      map_[*it] = TypeUpgrade(default_type, HEAP);
    }

    // function declarations
    typedef Scope::FunctionLiterals Functions;
    const Functions& functions = scope.function_declarations();
    for (Functions::const_iterator it = functions.begin(),
         last = functions.end(); it != last; ++it) {
      // TODO(Constellation) optimize it
      const Symbol sym = (*it)->name().Address()->symbol();
      // map_[sym] = default_type;
      map_[sym] = TypeUpgrade(default_type, HEAP);
    }
    // variables
    typedef Scope::Variables Variables;
    const Variables& vars = scope.variables();
    for (Variables::const_iterator it = vars.begin(),
         last = vars.end(); it != last; ++it) {
      const Symbol name = it->first->symbol();
      if (map_.find(name) != map_.end()) {
        map_[name] = TypeUpgrade(map_[name], default_type);
      } else {
        map_[name] = default_type;
      }
    }

    const FunctionLiteral::DeclType type = code->decl_type();
    if (type == FunctionLiteral::STATEMENT ||
        (type == FunctionLiteral::EXPRESSION && code_->HasName())) {
      const Symbol& name = code_->name();
      if (map_.find(name) == map_.end()) {
        map_[name] = TypeUpgrade(default_type, HEAP);
      }
    }
  }

  ~FunctionScope() {
    if (!upper_of_eval_) {
      // if arguments is realized, mark params to HEAP
      // TODO(Constellation) strict mode optimization
//      if (code_ && code_->ShouldCreateArguments()) {
//        for (Code::Names::const_iterator it = code->params().begin(),
//             last = code->params().end(); it != last; ++it) {
//          map_[*it] = TypeUpgrade(map_[*it], HEAP);
//        }
//      } else {
//        map_.erase(symbol::arguments);
//      }
      std::unordered_map<Symbol, uint16_t> locations;
      // dummy global
      if (code_) {
        uint16_t locals = 0;
        for (Variables::const_iterator it = map_.begin(),
             last = map_.end(); it != last; ++it) {
          if (it->second == STACK) {
            locations.insert(std::make_pair(it->first, locations.size()));
            Code::Names::iterator f =
                std::find(code_->varnames().begin(),
                          code_->varnames().end(), it->first);
            if (f != code_->varnames().end()) {
              code_->varnames().erase(f);
            }
            ++locals;
          }
        }
        code_->set_locals(locals);
        code_->set_stack_depth(code_->stack_depth() + locals);
      }
      for (Labels::const_iterator it = labels_.begin(),
           last = labels_.end(); it != last; ++it) {
        const Type type = map_[it->first];
        if (type == STACK) {
          const uint8_t op = (*data_)[it->second];
          (*data_)[it->second] = OP::ToLocal(op);
          const uint16_t loc = locations[it->first];
          (*data_)[it->second + 1] = (loc & 0xff);
          (*data_)[it->second + 2] = (loc >> 8);
        } else if (type == GLOBAL) {
          // emit global opt
          const uint8_t op = (*data_)[it->second];
          (*data_)[it->second] = OP::ToGlobal(op);
        }
      }
    }
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type) {
    if (map_.find(sym) == map_.end()) {
      if (IsTop()) {
        // this is global
        map_[sym] = TypeUpgrade((eval_top_scope_) ? LOOKUP : GLOBAL, type);
        labels_.push_back(std::make_pair(sym, target));
      } else {
        upper_->LookupImpl(sym, target, TypeUpgrade(type, HEAP));
      }
    } else {
      if (IsTop()) {
        map_[sym] = TypeUpgrade(map_[sym], type);
        labels_.push_back(std::make_pair(sym, target));
      } else {
        map_[sym] = TypeUpgrade(map_[sym], type);
        labels_.push_back(std::make_pair(sym, target));
      }
    }
  }

  virtual void MakeUpperOfEval() {
    upper_of_eval_ = true;
  }

  bool IsTop() const {
    return !upper_;
  }

 private:
  Variables map_;
  Labels labels_;
  Code* code_;
  Code::Data* data_;
  bool upper_of_eval_;
  bool eval_top_scope_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_SCOPE_H_
