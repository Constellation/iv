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
#include <iv/lv5/railgun/frame.h>
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

  static LookupInfo NewStack(int32_t register_location,
                             bool immutable,
                             const Assigned* assigned) {
    return LookupInfo(STACK,
                      register_location,
                      0,
                      immutable,
                      0,
                      assigned);
  }

  static LookupInfo NewUnused() {
    return LookupInfo(UNUSED, 0, 0, false, 0, NULL);
  }

  static LookupInfo NewLookup() {
    return LookupInfo(LOOKUP, 0, 0, false, 0, NULL);
  }

  static LookupInfo NewGlobal() {
    return LookupInfo(GLOBAL, 0, 0, false, 0, NULL);
  }

  static LookupInfo NewHeap(uint32_t heap_location,
                            int32_t register_location,
                            bool immutable,
                            uint32_t scope_nest_count,
                            const Assigned* assigned) {
    return LookupInfo(HEAP,
                      register_location,
                      heap_location,
                      immutable,
                      scope_nest_count,
                      assigned);
  }

  Type type() const { return type_; }

  int32_t register_location() const { return register_location_; }

  uint32_t heap_location() const { return heap_location_; }

  uint32_t scope() const { return scope_; }

  bool immutable() const { return immutable_; }

  const Assigned* assigned() const { return assigned_; }

  void Displace(int32_t heap, int32_t register_location) {
    assert(type_ == HEAP);
    heap_location_ = heap;
    // register_location_ = register_location;
  }

 private:
  LookupInfo(Type type,
             int32_t register_location,
             uint32_t heap_location,
             bool immutable,
             uint32_t scope,
             const Assigned* assigned)
    : type_(type),
      register_location_(register_location),
      heap_location_(heap_location),
      scope_(scope),
      immutable_(immutable),
      assigned_(assigned) {
  }

  Type type_;
  int32_t register_location_;
  uint32_t heap_location_;
  uint32_t scope_;
  bool immutable_;
  const Assigned* assigned_;
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
    return LookupInfo::NewLookup();
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
      return LookupInfo::NewLookup();
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

  typedef std::unordered_map<Symbol, LookupInfo> VariableMap;

  typedef std::vector<Symbol> HeapVariables;

  CodeScope(const FunctionLiteral* lit,
            const std::shared_ptr<VariableScope>& up,
            const Scope* scope,
            bool is_eval_decl)
    : VariableScope(
        up,
        (up->scope_nest_count() + (scope->needs_heap_scope() ? 1 : 0))),
      scope_(scope),
      literal_(lit),
      map_(),
      heap_(),
      stack_size_(0),
      is_eval_decl_(is_eval_decl) {
    assert(upper());

    // accumulate heaps for better heap numbers
    std::vector<Symbol> immutable_heaps;
    std::vector<Symbol> mutable_heaps;
    int32_t heap_size = 0;
    for (Scope::Assigneds::const_iterator it = scope->assigneds().begin(),
         last = scope->assigneds().end(); it != last; ++it) {
      // already unique
      assert(map_.find((*it)->symbol()) == map_.end());
      const Assigned* assigned = (*it);
      if (assigned->IsHeap() || scope_->upper_of_eval()) {
        // HEAP
        if (assigned->IsParameter()) {
          const Symbol name = assigned->symbol();
          const LookupInfo info = LookupInfo::NewHeap(
              heap_size,
              FrameConstant<>::ConvertArgToRegister(assigned->parameter()),
              assigned->immutable(),
              scope_nest_count(),
              assigned);
          ++heap_size;
          if (info.immutable()) {
            immutable_heaps.push_back(name);
          } else {
            mutable_heaps.push_back(name);
          }
          map_.insert(std::make_pair(name, info));
        } else {
          const Symbol name = assigned->symbol();
          const LookupInfo info = LookupInfo::NewHeap(
              heap_size,
              heap_size,
              assigned->immutable(),
              scope_nest_count(),
              assigned);
          ++heap_size;
          if (info.immutable()) {
            immutable_heaps.push_back(name);
          } else {
            mutable_heaps.push_back(name);
          }
          map_.insert(std::make_pair(name, info));
        }
      } else {
        // STACK
        if (assigned->IsReferenced()) {
          if (assigned->IsParameter()) {
            const LookupInfo info = LookupInfo::NewStack(
                FrameConstant<>::ConvertArgToRegister(assigned->parameter()),
                assigned->immutable(),
                assigned);
            map_.insert(std::make_pair(assigned->symbol(), info));
          } else {
            const LookupInfo info = LookupInfo::NewStack(
                stack_size_++,
                assigned->immutable(),
                assigned);
            map_.insert(std::make_pair(assigned->symbol(), info));
          }
        } else {
          map_.insert(
              std::make_pair(assigned->symbol(), LookupInfo::NewUnused()));
        }
      }
    }

    if (!is_eval_decl && map_.find(symbol::arguments()) == map_.end()) {
      // not hidden
      if (scope_->arguments_is_heap() || scope_->direct_call_to_eval()) {
        const Symbol name = symbol::arguments();
        const LookupInfo info = LookupInfo::NewHeap(
            heap_size,
            heap_size,
            scope->strict(),
            scope_nest_count(),
            NULL);
        ++heap_size;
        if (info.immutable()) {
          immutable_heaps.push_back(name);
        } else {
          mutable_heaps.push_back(name);
        }
        map_.insert(std::make_pair(name, info));
      } else if (scope_->has_arguments()) {
        const LookupInfo info =
            LookupInfo::NewStack(stack_size_++, scope->strict(), NULL);
        map_.insert(std::make_pair(symbol::arguments(), info));
      }
    }

    {
      // Displace Heaps
      int32_t i = 0;
      for (std::vector<Symbol>::const_iterator it = immutable_heaps.begin(),
           last = immutable_heaps.end(); it != last; ++it, ++i) {
        heap_.push_back(*it);
        map_.find(*it)->second.Displace(i, stack_size() + i);
      }
      mutable_start_ = i;
      for (std::vector<Symbol>::const_iterator it = mutable_heaps.begin(),
           last = mutable_heaps.end(); it != last; ++it, ++i) {
        heap_.push_back(*it);
        map_.find(*it)->second.Displace(i, stack_size() + i);
      }
    }
  }

  LookupInfo Lookup(Symbol sym) {
    const VariableMap::const_iterator it = map_.find(sym);
    if (it != map_.end()) {
      return it->second;
    } else {
      // not found in this scope
      if (scope_->direct_call_to_eval()) {
        // maybe new variable in this scope
        return LookupInfo::NewLookup();
      }
      return upper()->Lookup(sym);
    }
  }

  bool UseExpressionReturn() const { return is_eval_decl_; }

  bool LoadCalleeNeeded() const {
    if (literal_->IsFunctionNameExposed()) {
      const Symbol name = literal_->name().Address()->symbol();
      const VariableMap::const_iterator it = map_.find(name);
      assert(it != map_.end());
      if (const Assigned* assigned = it->second.assigned()) {
        if (assigned->function_name()) {
          return true;
        }
      }
    }
    return false;
  }

  const HeapVariables& heap() const { return heap_; }

  uint32_t stack_size() const { return stack_size_; }

  uint32_t heap_size() const { return heap_.size(); }

  int32_t mutable_start() const { return mutable_start_; }

  const Scope* scope() const { return scope_; }

 private:
  const Scope* scope_;
  const FunctionLiteral* literal_;
  VariableMap map_;
  HeapVariables heap_;
  uint32_t stack_size_;
  bool is_eval_decl_;
  int32_t mutable_start_;
};

typedef CodeScope<Code::EVAL> EvalScope;

template<>
class CodeScope<Code::EVAL> : public VariableScope {
 public:
  CodeScope() : VariableScope() { }

  CodeScope(const FunctionLiteral* literal,
            std::shared_ptr<VariableScope> upper,
            const Scope* scope,
            bool is_eval_decl)
    : VariableScope() {
  }

  bool UseExpressionReturn() const { return true; }

  LookupInfo Lookup(Symbol sym) {
    return LookupInfo::NewLookup();
  }
};

typedef CodeScope<Code::GLOBAL> GlobalScope;

template<>
class CodeScope<Code::GLOBAL> : public VariableScope {
 public:
  CodeScope() : VariableScope() { }

  CodeScope(const FunctionLiteral* lit,
            std::shared_ptr<VariableScope> upper,
            const Scope* scope,
            bool is_eval_decl)
    : VariableScope() {
  }

  LookupInfo Lookup(Symbol sym) {
    return LookupInfo::NewGlobal();
  }
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_SCOPE_H_
