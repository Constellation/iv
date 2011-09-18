#ifndef IV_LV5_AERO_AST_FWD_H_
#define IV_LV5_AERO_AST_FWD_H_
#define AERO_EXPRESSION_AST_DERIVED_NODES(V)\
  V(Disjunction)\
  V(Alternative)\
  V(HatAssertion)\
  V(DollarAssertion)\
  V(EscapedAssertion)\
  V(DisjunctionAssertion)\
  V(CharacterAtom)\
  V(DotAtom)\
  V(RangeAtom)\
  V(DisjunctionAtom)\
  V(Quantifiered)

#define AERO_EXPRESSION_AST_NODES(V)\
  AERO_EXPRESSION_AST_DERIVED_NODES(V)\
  V(Expression)\
  V(Term)\
  V(Atom)\
  V(Assertion)

namespace iv {
namespace lv5 {
namespace aero {

#define V(NODE) class NODE;
AERO_EXPRESSION_AST_NODES(V)
#undef V

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_AST_FWD_H_
