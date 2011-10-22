#ifndef IV_AERO_QUICK_CHECK_H_
#define IV_AERO_QUICK_CHECK_H_
#include "aero/quick_check_fwd.h"
#include "aero/compiler.h"
namespace iv {
namespace aero {

inline void QuickCheck::Emit(const ParsedData& data) {
  data.pattern()->Accept(this);
}

inline void QuickCheck::Visit(Disjunction* dis) {
  for (Alternatives::const_iterator it = dis->alternatives().begin(),
       last = dis->alternatives().end(); it != last; ++it) {
    (*it)->Accept(this);
  }
}

inline void QuickCheck::Visit(Alternative* alt) {
  for (Expressions::const_iterator it = alt->terms().begin(),
       last = alt->terms().end(); it != last; ++it) {
    (*it)->Accept(this);
  }
}

inline void QuickCheck::Visit(HatAssertion* assertion) { }

inline void QuickCheck::Visit(DollarAssertion* assertion) { }

inline void QuickCheck::Visit(EscapedAssertion* assertion) { }

inline void QuickCheck::Visit(DisjunctionAssertion* assertion) {
  assertion->disjunction()->Accept(this);
}

inline void QuickCheck::Visit(BackReferenceAtom* atom) { }

inline void QuickCheck::Visit(CharacterAtom* atom) { }

inline void QuickCheck::Visit(RangeAtom* atom) { }

inline void QuickCheck::Visit(DisjunctionAtom* atom) {
  Visit(atom->disjunction());
}

inline void QuickCheck::Visit(Quantifiered* atom) {
  atom->expression()->Accept(this);
}

} }  // namespace iv::aero
#endif  // IV_AERO_QUICK_CHECK_H_
