#ifndef IV_ENVIRONMENT_H_
#define IV_ENVIRONMENT_H_
#include <iv/detail/unordered_map.h>
#include <iv/detail/type_traits.h>
#include <iv/noncopyable.h>
#include <iv/symbol.h>
#include <iv/ast.h>
namespace iv {
namespace core {

class Environment : private Noncopyable<Environment> {
 public:
  // boolean value represents it is heap saved.
  typedef std::unordered_map<Symbol, bool> SymbolMap;

  ~Environment() {
    *placeholder_ = upper();
  }

  virtual void Propagate(SymbolMap* map) = 0;

  void Referencing(Symbol sym) {
    // first, so this is stack
    Referencing(sym, with_environment());
  }

  bool with_environment() const { return with_environment_; }

  virtual void Referencing(Symbol symbol, bool type) = 0;

  Environment* upper() const { return upper_; }

  uint32_t scope_nest_count() const { return scope_nest_count_; }

  void RecordDirectCallToEval() {
    direct_call_to_eval_ = true;
    RecordUpperOfEval();
    Environment* env = upper();
    while (env) {
      env->RecordUpperOfEval();
      env = env->upper();
    }
  }

 protected:
  Environment(Environment* upper,
              Environment** placeholder,
              bool with_environment = false)
    : upper_(upper),
      placeholder_(placeholder),
      with_environment_(with_environment),
      eval_environment_(false),
      direct_call_to_eval_(false),
      upper_of_eval_(false),
      scope_nest_count_(upper ? (upper->scope_nest_count() + 1) : 0) {
    *placeholder_ = this;
  }

 private:
  void RecordUpperOfEval() {
    upper_of_eval_ = true;
  }

  Environment* upper_;
  Environment** placeholder_;
  bool with_environment_;
  bool eval_environment_;
  bool direct_call_to_eval_;
  bool upper_of_eval_;
  uint32_t scope_nest_count_;
};

class WithEnvironment : public Environment {
 public:
  WithEnvironment(Environment* upper, Environment** placeholder)
    : Environment(upper, placeholder, true) {
    assert(upper);
  }

  void Propagate(SymbolMap* map) {
    upper()->Propagate(map);
  }

  void Referencing(Symbol symbol, bool type) {
    upper()->Referencing(symbol, true);
  }
};

class CatchEnvironment : public Environment {
 public:
  CatchEnvironment(Environment* upper, Environment** placeholder, Symbol sym)
    : Environment(upper, placeholder),
      target_(sym) {
    assert(upper);
  }

  void Propagate(SymbolMap* map) {
    map->erase(target_);  // trap it by this environment
    upper()->Propagate(map);
  }

  void Referencing(Symbol symbol, bool type) {
    if (symbol != target_) {
      // trapped by this environment
      upper()->Referencing(symbol, type);
    }
  }
 private:
  Symbol target_;
};

class FunctionEnvironment : public Environment {
 public:
  FunctionEnvironment(Environment* upper,
                      Environment** placeholder)
    : Environment(upper, placeholder),
      unresolved_() {
  }

  template<typename ScopeType>
  void Resolve(ScopeType* scope) {
    typedef typename std::remove_pointer<
        typename ScopeType::Assigneds::value_type>::type Assigned;
    typedef std::unordered_map<Symbol, Assigned*> AssignedMap;
    AssignedMap map;
    // construct map
    for (typename ScopeType::Assigneds::const_iterator
         it = scope->assigneds().begin(),
         last = scope->assigneds().end();
         it != last; ++it) {
      map.insert(std::make_pair((*it)->symbol(), *it));
    }

    // resolve reference
    for (typename AssignedMap::const_iterator it = map.begin(),
         last = map.end(); it != last; ++it) {
      const typename SymbolMap::iterator found = unresolved_.find(it->first);
      if (found != unresolved_.end()) {
        // resolve this
        it->second->set_type(found->second);
        unresolved_.erase(found);
      }
    }
    if (upper()) {
      upper()->Propagate(&unresolved_);
    }
  }

  void Referencing(Symbol symbol, bool type) {
    const SymbolMap::iterator it = unresolved_.find(symbol);
    if (it != unresolved_.end()) {
      it->second |= type;
    } else {
      unresolved_.insert(std::make_pair(symbol, type));
    }
  }

  void Propagate(SymbolMap* map) {
    for (SymbolMap::const_iterator it = map->begin(),
         last = map->end(); it != last; ++it) {
      unresolved_[it->first] = true;
    }
  }
 private:
  SymbolMap unresolved_;
};

} }  // namespace iv::core
#endif  // IV_ENVIRONMENT_H_
