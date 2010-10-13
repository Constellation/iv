#ifndef _IV_SCOPE_H_
#define _IV_SCOPE_H_
#include <vector>
#include "noncopyable.h"
#include "alloc-inl.h"
namespace iv {
namespace core {

class Declaration;
class Identifier;
class Expression;
class FunctionLiteral;

class Scope : public SpaceObject, private Noncopyable<Scope>::type  {
 public:
  typedef std::pair<Identifier*, bool> Variable;
  typedef SpaceVector<Variable>::type Variables;
  typedef SpaceVector<FunctionLiteral*>::type FunctionLiterals;

  explicit Scope(Space* factory)
    : up_(NULL),
      vars_(Variables::allocator_type(factory)),
      funcs_(FunctionLiterals::allocator_type(factory)) {
  }
  ~Scope() { }
  void AddUnresolved(Identifier* name, bool is_const) {
    vars_.push_back(Variables::value_type(name, is_const));
  }
  void AddFunctionDeclaration(FunctionLiteral* func);
  void SetUpperScope(Scope* scope);
  inline const FunctionLiterals& function_declarations() const {
    return funcs_;
  }
  inline const Variables& variables() const {
    return vars_;
  }
  Scope* GetUpperScope();
  bool Contains(const Identifier& ident) const;
 protected:
  Scope* up_;
  Variables vars_;
  FunctionLiterals funcs_;
};

} }  // namespace iv::core
#endif  // _IV_SCOPE_H_
