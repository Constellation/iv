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
#include "lv5/railgun/direct_threading.h"
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

  void Lookup(Symbol sym, std::size_t target, Code* from) {
    // first, so this is stack
    LookupImpl(sym, target, (InWith())? LOOKUP : STACK, from);
  }

  virtual bool InWith() const {
    return false;
  }

  virtual bool IsEvalScope() const {
    return false;
  }

  static Type TypeUpgrade(Type now, Type next) {
    return std::max<Type>(now, next);
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type, Code* from) = 0;

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

  bool InWith() const {
    return true;
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type, Code* from) {
    upper_->LookupImpl(sym, target, TypeUpgrade(type, LOOKUP), from);
  }
};

class CatchScope : public VariableScope {
 public:
  CatchScope(std::shared_ptr<VariableScope> upper, Symbol sym)
    : VariableScope(upper),
      target_(sym) {
    assert(upper);
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type, Code* from) {
    if (sym != target_) {
      upper_->LookupImpl(sym, target, TypeUpgrade(type, HEAP), from);
    }
  }

 private:
  Symbol target_;
};

class FunctionScope : public VariableScope {
 public:

  // symbol, point, type, from
  typedef std::vector<std::tuple<Symbol, std::size_t, Type, Code*> > Labels;
  // type, refcount, immutable
  typedef std::tuple<Type, std::size_t, bool> Variable;
  typedef std::unordered_map<Symbol, Variable> Variables;
  // Symbol -> used, location
  typedef std::unordered_map<Symbol, std::tuple<bool, uint32_t> > Locations;

  FunctionScope(std::shared_ptr<VariableScope> upper, Code::Data* data)
    : VariableScope(upper),
      map_(),
      labels_(),
      code_(NULL),
      data_(data),
      upper_of_eval_(false),
      eval_top_scope_(false),
      eval_target_scope_(false) {
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
      eval_top_scope_(code->code_type() == Code::EVAL),
      eval_target_scope_(scope.HasDirectCallToEval()) {
    if (!IsTop()) {
      // is global or not
      map_[symbol::arguments()] = std::make_tuple(STACK, 0, code_->strict());

      // params
      for (Code::Names::const_iterator it = code->params().begin(),
           last = code->params().end(); it != last; ++it) {
        map_[*it] = std::make_tuple(STACK, 0, false);
      }

      // function declarations
      typedef Scope::FunctionLiterals Functions;
      const Functions& functions = scope.function_declarations();
      for (Functions::const_iterator it = functions.begin(),
           last = functions.end(); it != last; ++it) {
        const Symbol sym = (*it)->name().Address()->symbol();
        map_[sym] = std::make_tuple(STACK, 0, false);
      }

      // variables
      typedef Scope::Variables Variables;
      const Variables& vars = scope.variables();
      for (Variables::const_iterator it = vars.begin(),
           last = vars.end(); it != last; ++it) {
        const Symbol name = it->first->symbol();
        if (map_.find(name) != map_.end()) {
          map_[name] = std::make_tuple(TypeUpgrade(std::get<0>(map_[name]), STACK), 0, false);
        } else {
          map_[name] = std::make_tuple(STACK, 0, false);
        }
      }

      const FunctionLiteral::DeclType type = code->decl_type();
      if (type == FunctionLiteral::STATEMENT ||
          (type == FunctionLiteral::EXPRESSION && code_->HasName())) {
        const Symbol& name = code_->name();
        if (map_.find(name) == map_.end()) {
          map_[name] = std::make_tuple(STACK, 0, true);
        }
      }
    }
  }

  ~FunctionScope() {
    if (IsTop()) {
      if (!eval_top_scope_) {
        // if direct call to eval top scope
        // all variable is {configurable : true}, so not STACK
        for (Labels::const_iterator it = labels_.begin(),
             last = labels_.end(); it != last; ++it) {
          const Symbol sym = std::get<0>(*it);
          const std::size_t point = std::get<1>(*it);
          const Type type = TypeUpgrade(std::get<0>(map_[sym]), std::get<2>(*it));
          if (type == GLOBAL) {
            // emit global opt
            const uint32_t op = (*data_)[point].value;
            (*data_)[point] = OP::ToGlobal(op);
          }
        }
      }
    } else {
      assert(code_);
      Locations locations;
      uint32_t location = 0;
      if (!upper_of_eval_) {
        if (code_->ShouldCreateArguments() && !code_->strict()) {
          for (Code::Names::const_iterator it = code_->params().begin(),
               last = code_->params().end(); it != last; ++it) {
            std::get<0>(map_[*it]) = TypeUpgrade(std::get<0>(map_[*it]), HEAP);
          }
        }
        for (Variables::const_iterator it = map_.begin(),
             last = map_.end(); it != last; ++it) {
          if (std::get<0>(it->second) == STACK) {
            if (std::get<1>(it->second) == 0) {  // ref count is 0
              locations.insert(std::make_pair(it->first, std::make_tuple(false, 0u)));
            } else {
              locations.insert(std::make_pair(it->first, std::make_tuple(true, location++)));
              code_->locals_.push_back(it->first);
            }
          }
        }
        code_->set_stack_depth(code_->stack_depth() + code_->locals().size());

        for (Labels::const_iterator it = labels_.begin(),
             last = labels_.end(); it != last; ++it) {
          const Symbol sym = std::get<0>(*it);
          const std::size_t point = std::get<1>(*it);
          const Type type = TypeUpgrade(std::get<0>(map_[sym]), std::get<2>(*it));
          if (type == STACK) {
            const bool immutable = std::get<2>(map_[sym]);
            Code* from = std::get<3>(*it);
            const uint32_t op = (*data_)[point].value;
            (*data_)[point] = (immutable) ?
                OP::ToLocalImmutable(op, from->strict()) : OP::ToLocal(op);
            assert(locations.find(sym) != locations.end());
            assert(std::get<0>(locations[sym]));
            const uint32_t loc = std::get<1>(locations[sym]);
            (*data_)[point + 1] = loc;
          } else if (type == GLOBAL) {
            // emit global opt
            const uint32_t op = (*data_)[point].value;
            (*data_)[point] = OP::ToGlobal(op);
          }
        }
      }
      CleanUpDecls(code_, locations);
    }
  }

  virtual void LookupImpl(Symbol sym, std::size_t target, Type type, Code* from) {
    if (map_.find(sym) == map_.end()) {
      if (IsTop()) {
        // this is global
        map_[sym] = std::make_tuple((eval_top_scope_) ? LOOKUP : GLOBAL, 1, false);
        labels_.push_back(
            std::make_tuple(
                sym,
                target,
                TypeUpgrade((eval_top_scope_) ? LOOKUP : GLOBAL, type),
                from));
      } else {
        upper_->LookupImpl(sym, target, TypeUpgrade(type, (IsEvalScope()) ? LOOKUP : HEAP), from);
      }
    } else {
      if (IsTop()) {
        ++std::get<1>(map_[sym]);
        labels_.push_back(
            std::make_tuple(
                sym,
                target,
                TypeUpgrade((eval_top_scope_) ? LOOKUP : GLOBAL, type),
                from));
      } else {
        const Type stored = (type == LOOKUP || type == GLOBAL) ? HEAP : type;
        std::get<0>(map_[sym]) = TypeUpgrade(std::get<0>(map_[sym]), stored);
        ++std::get<1>(map_[sym]);
        labels_.push_back(
            std::make_tuple(
                sym,
                target,
                type,
                from));
      }
    }
  }

  virtual void MakeUpperOfEval() {
    upper_of_eval_ = true;
  }

  virtual bool IsEvalScope() const {
    return eval_target_scope_;
  }

  bool IsTop() const {
    return !upper_;
  }

 private:
  class SearchDecl {
   public:
    explicit SearchDecl(Symbol sym) : sym_(sym) { }
    template<typename DeclTuple>
    bool operator()(const DeclTuple& decl) const {
      return std::get<0>(decl) == sym_;
    }
   private:
   Symbol sym_;
  };

  void CleanUpDecls(Code* code,
                    const Locations& locations) {
    bool needs_env = false;
    std::unordered_set<Symbol> already_decled;
    {
      std::size_t param_count = 0;
      std::unordered_map<Symbol, std::size_t> sym2c;
      for (Code::Names::const_iterator it = code->params().begin(),
           last = code->params().end(); it != last; ++it, ++param_count) {
        sym2c[*it] = param_count;
      }
      for (std::unordered_map<Symbol, std::size_t>::const_iterator it = sym2c.begin(),
           last = sym2c.end(); it != last; ++it) {
        const Symbol sym = it->first;
        const Locations::const_iterator f = locations.find(sym);
        if (f == locations.end()) {
          needs_env = true;
          code->decls_.push_back(
              std::make_tuple(
                  sym,
                  Code::PARAM,
                  it->second,
                  0u));
        } else {
          // PARAM on STACK
          if (std::get<0>(f->second)) {  // used
            code->decls_.push_back(
                std::make_tuple(
                    sym,
                    Code::PARAM_LOCAL,
                    it->second,
                    std::get<1>(f->second)));
          }
        }
        already_decled.insert(sym);
      }
    }

    for (Code::Codes::const_iterator it = code->codes().begin(),
         last = code->codes().end(); it != last; ++it) {
      if ((*it)->IsFunctionDeclaration()) {
        const Symbol& fn = (*it)->name();
        if (locations.find(fn) == locations.end() &&
            already_decled.find(fn) == already_decled.end()) {
          needs_env = true;
          code->decls_.push_back(
              std::make_tuple(fn, Code::FDECL, 0, 0u));
          already_decled.insert(fn);
        }
      }
    }

    if (code->ShouldCreateArguments()) {
      const Locations::const_iterator f = locations.find(symbol::arguments());
      if (f == locations.end()) {
        needs_env = true;
        code->decls_.push_back(
            std::make_tuple(symbol::arguments(), Code::ARGUMENTS, 0, 0u));
      } else {
        assert(std::get<0>(f->second));  // used
        code->decls_.push_back(
            std::make_tuple(symbol::arguments(), Code::ARGUMENTS_LOCAL, 0, std::get<1>(f->second)));
      }
      already_decled.insert(symbol::arguments());
    }

    for (Code::Names::const_iterator it = code->varnames().begin(),
         last = code->varnames().end(); it != last; ++it) {
      const Symbol& dn = *it;
      if (locations.find(dn) == locations.end() &&
          already_decled.find(dn) == already_decled.end()) {
        needs_env = true;
        code->decls_.push_back(std::make_tuple(dn, Code::VAR, 0, 0u));
        already_decled.insert(dn);
      }
    }

    const FunctionLiteral::DeclType type = code->decl_type();
    if (type == FunctionLiteral::STATEMENT ||
        (type == FunctionLiteral::EXPRESSION && code->HasName())) {
      const Symbol& fn = code->name();
      if (already_decled.find(fn) == already_decled.end()) {
        const Locations::const_iterator f = locations.find(fn);
        if (f == locations.end()) {
          needs_env = true;
          code->decls_.push_back(std::make_tuple(fn, Code::FEXPR, 0, 0u));
        } else {
          if (std::get<0>(f->second)) {  // used
            code->decls_.push_back(std::make_tuple(fn, Code::FEXPR_LOCAL, 0, std::get<1>(f->second)));
          }
        }
        already_decled.insert(fn);
      }
    }
    if (!needs_env) {
      code->set_has_declarative_env(false);
    }
  }

  Variables map_;
  Labels labels_;
  Code* code_;
  Code::Data* data_;
  bool upper_of_eval_;
  bool eval_top_scope_;
  bool eval_target_scope_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_SCOPE_H_
