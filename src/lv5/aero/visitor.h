#ifndef IV_LV5_AERO_VISITOR_H_
#define IV_LV5_AERO_VISITOR_H_
#include "lv5/aero/ast_fwd.h"
namespace iv {
namespace lv5 {
namespace aero {

class Visitor {
 public:
  virtual ~Visitor() { }
#define V(NODE) virtual void Visit(NODE* node) = 0;
AERO_EXPRESSION_AST_DERIVED_NODES(V)
#undef V
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_VISITOR_H_
