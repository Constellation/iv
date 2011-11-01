#ifndef IV_AERO_VISITOR_H_
#define IV_AERO_VISITOR_H_
#include <iv/aero/ast_fwd.h>
namespace iv {
namespace aero {

class Visitor {
 public:
  virtual ~Visitor() { }
#define V(NODE) virtual void Visit(NODE* node) = 0;
AERO_EXPRESSION_AST_DERIVED_NODES(V)
#undef V
};

} }  // namespace iv::aero
#endif  // IV_AERO_VISITOR_H_
