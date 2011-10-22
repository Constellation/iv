#ifndef IV_AERO_AST_FWD_H_
#define IV_AERO_AST_FWD_H_
#define AERO_EXPRESSION_AST_DERIVED_NODES(V)\
  V(Disjunction)\
  V(Alternative)\
  V(HatAssertion)\
  V(DollarAssertion)\
  V(EscapedAssertion)\
  V(DisjunctionAssertion)\
  V(BackReferenceAtom)\
  V(CharacterAtom)\
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
namespace aero {

#define V(NODE) class NODE;
AERO_EXPRESSION_AST_NODES(V)
#undef V

} }  // namespace iv::aero
#endif  // IV_AERO_AST_FWD_H_
