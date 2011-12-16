#ifndef IV_LV5_RAILGUN_SCOPE_H_
#define IV_LV5_RAILGUN_SCOPE_H_
#include <algorithm>
#include <iv/detail/memory.h>
#include <iv/detail/unordered_map.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/code.h>
#include <iv/lv5/railgun/direct_threading.h>
namespace iv {
namespace lv5 {
namespace railgun {

class LookupInfo {
 public:
  enum Type {
    STACK,
    HEAP,
    GLOBAL,
    LOOKUP,
    UNUSED
  };

  LookupInfo(Type type,
             uint32_t location = 0,
             bool immutable = false,
             uint32_t scope = 0)
    : type_(type),
      location_(location),
      scope_(scope),
      immutable_(immutable) {
  }

  Type type() const { return type_; }

  uint32_t location() const { return location_; }

  uint32_t scope() const { return scope_; }

  bool immutable() const { return immutable_; }

 private:
  Type type_;
  uint32_t location_;
  uint32_t scope_;
  bool immutable_;
};

class VariableScope : private core::Noncopyable<VariableScope> {
 public:
  virtual ~VariableScope() { }

  const std::shared_ptr<VariableScope>& upper() const {
    return upper_;
  }

  virtual LookupInfo Lookup(Symbol sym) = 0;

  uint32_t scope_nest_count() const {
    return scope_nest_count_;
  }

  virtual bool UseExpressionReturn() const { return false; }

 protected:
  // from catch or with
  explicit VariableScope(const std::shared_ptr<VariableScope>& upper)
    : upper_(upper),
      scope_nest_count_(upper->scope_nest_count() + 1) {
    // automatically count up nest
  }

  // from function
  VariableScope(const std::shared_ptr<VariableScope>& upper, uint32_t nest)
    : upper_(upper),
      scope_nest_count_(nest) {
  }

  // from eval or global
  VariableScope() : upper_(), scope_nest_count_(0) { }

 private:
  std::shared_ptr<VariableScope> upper_;
  uint32_t scope_nest_count_;
};

class WithScope : public VariableScope {
 public:
  explicit WithScope(const std::shared_ptr<VariableScope>& upper)
    : VariableScope(upper) {
    assert(upper);
  }

  LookupInfo Lookup(Symbol sym) {
    return LookupInfo(LookupInfo::LOOKUP);
  }

  bool UseExpressionReturn() const { return upper()->UseExpressionReturn(); }
};

class CatchScope : public VariableScope {
 public:
  CatchScope(const std::shared_ptr<VariableScope>& upper, Symbol sym)
    : VariableScope(upper),
      target_(sym) {
    assert(upper);
  }

  LookupInfo Lookup(Symbol sym) {
    if (sym == target_) {
      return LookupInfo(LookupInfo::LOOKUP);
    } else {
      return upper()->Lookup(sym);
    }
  }

  bool UseExpressionReturn() const { return upper()->UseExpressionReturn(); }

 private:
  Symbol target_;
};

template<Code::CodeType TYPE>
class CodeScope;

typedef CodeScope<Code::FUNCTION> FunctionScope;

template<>
class CodeScope<Code::FUNCTION> : public VariableScope {
 public:
  // Variable is tuple<heap, location, immutable>
  enum Type {
    STACK  = 0,
    HEAP   = 1,
    UNUSED = 2
  };
  typedef std::tuple<Type, uint32_t, bool> Variable;
  typedef std::vector<std::pair<Symbol, Variable> > HeapVariables;
  typedef std::unordered_map<Symbol, Variable> VariableMap;

  CodeScope(const std::shared_ptr<VariableScope>& up,
            const Scope* scope,
            bool is_eval_decl)
    : VariableScope(
        up,
        (up->scope_nest_count() + (scope->needs_heap_scope() ? 1 : 0))),
      scope_(scope),
      map_(),
      heap_(),
      stack_size_(0),
      is_eval_decl_(is_eval_decl) {
    assert(upper());
    for (Scope::Assigneds::const_iterator it = scope->assigneds().begin(),
         last = scope->assigneds().end(); it != last; ++it) {
      // already unique
      assert(map_.find((*it)->symbol()) == map_.end());
      const Assigned* assigned = (*it);
      if (assigned->IsHeap() || scope_->upper_of_eval()) {
        const std::pair<Symbol, Variable> item =
            std::make_pair(
                assigned->symbol(),
                std::make_tuple(HEAP, heap_.size(), assigned->immutable()));
        heap_.push_back(item);
        map_.insert(item);
      } else {
        if (assigned->IsReferenced()) {
          const std::pair<Symbol, Variable> item =
              std::make_pair(
                  assigned->symbol(),
                  std::make_tuple(STACK, stack_size_++, assigned->immutable()));
          map_.insert(item);
        } else {
          const std::pair<Symbol, Variable> item =
              std::make_pair(
                  assigned->symbol(),
                  std::make_tuple(UNUSED, 0, assigned->immutable()));
          map_.insert(item);
        }
      }
    }

    if (!is_eval_decl && map_.find(symbol::arguments()) == map_.end()) {
      // not hidden
      if (scope_->arguments_is_heap() || scope_->direct_call_to_eval()) {
        const std::pair<Symbol, Variable> item =
            std::make_pair(
                symbol::arguments(),
                std::make_tuple(HEAP, heap_.size(), scope->strict()));
        heap_.push_back(item);
        map_.insert(item);
      } else if (scope_->has_arguments()) {
        const std::pair<Symbol, Variable> item =
            std::make_pair(
                symbol::arguments(),
                std::make_tuple(STACK, stack_size_++, scope->strict()));
        map_.insert(item);
      }
    }
  }

  LookupInfo Lookup(Symbol sym) {
    const VariableMap::const_iterator it = map_.find(sym);
    if (it != map_.end()) {
      const Type type = std::get<0>(it->second);
      const uint32_t location = std::get<1>(it->second);
      const bool immutable = std::get<2>(it->second);
      if (type == HEAP) {
        return LookupInfo(LookupInfo::HEAP, location,
                          immutable, scope_nest_count());
      } else if (type == STACK) {
        return LookupInfo(LookupInfo::STACK, location, immutable);
      } else {
        return LookupInfo(LookupInfo::UNUSED);
      }
    } else {
      // not found in this scope
      if (scope_->direct_call_to_eval()) {
        // maybe new variable in this scope
        return LookupInfo(LookupInfo::LOOKUP);
      }
      return upper()->Lookup(sym);
    }
  }

  bool UseExpressionReturn() const { return is_eval_decl_; }

  const HeapVariables& heap() const { return heap_; }

  uint32_t stack_size() const { return stack_size_; }

  uint32_t heap_size() const { return heap_.size(); }

  const Scope* scope() const { return scope_; }

 private:
  const Scope* scope_;
  VariableMap map_;
  HeapVariables heap_;
  uint32_t stack_size_;
  bool is_eval_decl_;
};

typedef CodeScope<Code::EVAL> EvalScope;

template<>
class CodeScope<Code::EVAL> : public VariableScope {
 public:
  CodeScope() : VariableScope() { }

  CodeScope(std::shared_ptr<VariableScope> upper,
            const Scope* scope,
            bool is_eval_decl)
    : VariableScope() {
  }

  bool UseExpressionReturn() const { return true; }

  LookupInfo Lookup(Symbol sym) {
    return LookupInfo(LookupInfo::LOOKUP);
  }
};

typedef CodeScope<Code::GLOBAL> GlobalScope;

template<>
class CodeScope<Code::GLOBAL> : public VariableScope {
 public:
  CodeScope() : VariableScope() { }

  CodeScope(std::shared_ptr<VariableScope> upper,
            const Scope* scope,
            bool is_eval_decl)
    : VariableScope() {
  }

  LookupInfo Lookup(Symbol sym) {
    return LookupInfo(LookupInfo::GLOBAL);
  }
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_SCOPE_H_
