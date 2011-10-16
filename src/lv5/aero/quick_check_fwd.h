#ifndef IV_LV5_AERO_QUICK_CHECK_FWD_H_
#define IV_LV5_AERO_QUICK_CHECK_FWD_H_
#include "lv5/aero/visitor.h"
namespace iv {
namespace lv5 {
namespace aero {

class Compiler;

class QuickCheck : private Visitor {
 public:
  explicit QuickCheck(Compiler* compiler) : compiler_(compiler) { }
  void Emit(const ParsedData& data);
 private:
  void Visit(Disjunction* dis);
  void Visit(Alternative* alt);
  void Visit(HatAssertion* assertion);
  void Visit(DollarAssertion* assertion);
  void Visit(EscapedAssertion* assertion);
  void Visit(DisjunctionAssertion* assertion);
  void Visit(BackReferenceAtom* atom);
  void Visit(CharacterAtom* atom);
  void Visit(RangeAtom* atom);
  void Visit(DisjunctionAtom* atom);
  void Visit(Quantifiered* atom);

  Compiler* compiler_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_QUICK_CHECK_FWD_H_
