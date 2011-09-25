// dumper for debug
#ifndef IV_LV5_AERO_DUMPER_H_
#define IV_LV5_AERO_DUMPER_H_
#include "string_builder.h"
#include "lv5/aero/visitor.h"
#include "lv5/aero/ast.h"
namespace iv {
namespace lv5 {
namespace aero {

class Dumper : public Visitor {
 public:
  Dumper() : builder_() { }
  core::UString Dump(Expression* node) {
    builder_.clear();
    node->Accept(this);
    return builder_.Build();
  }
 private:
  void Visit(Disjunction* dis) {
    builder_.Append("DIS(");
    for (Alternatives::const_iterator it = dis->alternatives().begin(),
         last = dis->alternatives().end(); it != last;) {
      Visit(*it);
      ++it;
      if (it != last) {
        builder_.Append('|');
      }
    }
    builder_.Append(')');
  }
  void Visit(Alternative* alt) {
    builder_.Append("ALT(");
    for (Expressions::const_iterator it = alt->terms().begin(),
         last = alt->terms().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
    builder_.Append(')');
  }
  void Visit(HatAssertion* assertion) {
    builder_.Append("AS(^)");
  }
  void Visit(DollarAssertion* assertion) {
    builder_.Append("AS($)");
  }
  void Visit(EscapedAssertion* assertion) {
    builder_.Append("AS(");
    if (assertion->uppercase()) {
      builder_.Append('B');
    } else {
      builder_.Append('b');
    }
    builder_.Append(")");
  }
  void Visit(DisjunctionAssertion* assertion) {
    builder_.Append("AS(");
    if (assertion->inverted()) {
      builder_.Append('!');
    } else {
      builder_.Append('=');
    }
    assertion->disjunction()->Accept(this);
    builder_.Append(')');
  }
  void Visit(CharacterAtom* atom) {
    builder_.Append(atom->character());
  }
  void Visit(RangeAtom* atom) {
    // TODO(Constellation): implement it
  }
  void Visit(DisjunctionAtom* atom) {
    if (atom->captured()) {
      builder_.Append("AT(");
    } else {
      builder_.Append("AT(:");
    }
    atom->disjunction()->Accept(this);
    builder_.Append(')');
  }
  void Visit(Quantifiered* atom) {
    builder_.Append("Q(");
    if (atom->greedy()) {
      builder_.Append('G');
    }
    builder_.Append(':');
    core::Int32ToString(atom->min(), std::back_inserter(builder_));
    builder_.Append(':');
    core::Int32ToString(atom->max(), std::back_inserter(builder_));
    builder_.Append(':');
    atom->expression()->Accept(this);
    builder_.Append(')');
  }

  core::UStringBuilder builder_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_DUMPER_H_
