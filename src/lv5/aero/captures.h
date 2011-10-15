#ifndef IV_LV5_AERO_CAPTURES_H_
#define IV_LV5_AERO_CAPTURES_H_
#include "lv5/aero/visitor.h"
#include "lv5/aero/ast.h"
namespace iv {
namespace lv5 {
namespace aero {

class CaptureCalculator : public Visitor {
 public:
  explicit CaptureCalculator(uint32_t now) : capture_(now) { }
  uint32_t Calculate(Expression* expr) {
    expr->Accept(this);
    return capture_;
  }
 private:
  void Visit(Disjunction* dis) {
    for (Alternatives::const_iterator it = dis->alternatives().begin(),
         last = dis->alternatives().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }
  void Visit(Alternative* alt) {
    for (Expressions::const_iterator it = alt->terms().begin(),
         last = alt->terms().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }
  void Visit(HatAssertion* assertion) { }
  void Visit(DollarAssertion* assertion) { }
  void Visit(EscapedAssertion* assertion) { }
  void Visit(DisjunctionAssertion* assertion) {
    assertion->disjunction()->Accept(this);
  }
  void Visit(BackReferenceAtom* atom) { }
  void Visit(CharacterAtom* atom) { }
  void Visit(RangeAtom* atom) { }
  void Visit(DisjunctionAtom* atom) {
    if (atom->captured()) {
      capture_ = std::max(capture_, atom->num());
    }
    Visit(atom->disjunction());
  }
  void Visit(Quantifiered* atom) {
    atom->expression()->Accept(this);
  }
  uint32_t capture_;
};

inline uint32_t GainNextCaptures(Expression* expr, uint32_t now) {
  return CaptureCalculator(now).Calculate(expr);
}

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_CAPTURES_H_
