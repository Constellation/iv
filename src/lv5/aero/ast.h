#ifndef IV_LV5_AERO_AST_H_
#define IV_LV5_AERO_AST_H_
#include "space.h"
#include "lv5/aero/visitor.h"
#include "lv5/aero/range.h"
namespace iv {
namespace lv5 {
namespace aero {

static const int kRegExpInfinity = std::numeric_limits<int>::max();

#define ACCEPT_VISITOR\
  inline void Accept(Visitor* visitor) {\
    visitor->Visit(this);\
  }

#define DECLARE_NODE_TYPE(type)\
  inline const type* As##type() const { return this; }\
  inline type* As##type() { return this; }

#define DECLARE_DERIVED_NODE_TYPE(type)\
  DECLARE_NODE_TYPE(type)\
  ACCEPT_VISITOR

#define DECLARE_NODE_TYPE_BASE(type)\
  inline virtual const type* As##type() const { return NULL; }\
  inline virtual type* As##type() { return NULL; }

class Expression : public core::SpaceObject {
 public:
  virtual ~Expression() { }
  AERO_EXPRESSION_AST_NODES(DECLARE_NODE_TYPE_BASE)
  virtual void Accept(Visitor* visitor) = 0;
};

inline void Expression::Accept(Visitor* visitor) { }

typedef core::SpaceVector<core::Space, Expression*>::type Expressions;
typedef core::SpaceVector<core::Space, Alternative*>::type Alternatives;
typedef core::SpaceVector<core::Space, Range>::type Ranges;

class Disjunction : public Expression {
 public:
  explicit Disjunction(Alternatives* alternatives)
    : alternatives_(alternatives) { }
  const Alternatives& alternatives() const {
    return *alternatives_;
  }
  DECLARE_DERIVED_NODE_TYPE(Disjunction)
 private:
  Alternatives* alternatives_;
};

class Alternative : public Expression {
 public:
  explicit Alternative(Expressions* terms) : terms_(terms) { }
  const Expressions& terms() const { return *terms_; }
  DECLARE_DERIVED_NODE_TYPE(Alternative)
 private:
  Expressions* terms_;
};

class Term : public Expression {
 public:
  DECLARE_NODE_TYPE(Term)
};

class Atom : public Term {
 public:
  DECLARE_NODE_TYPE(Atom)
};

class Quantifiered : public Atom {
 public:
  Quantifiered(Expression* expr, int min, int max, bool greedy)
    : expression_(expr), min_(min), max_(max), greedy_(greedy) { }
  Expression* expression() const { return expression_; }
  int min() const { return min_; }
  int max() const { return max_; }
  bool greedy() const { return greedy_; }
  DECLARE_DERIVED_NODE_TYPE(Quantifiered)
 private:
  Expression* expression_;
  int min_;
  int max_;
  bool greedy_;
};

class Assertion : public Term {
 public:
  DECLARE_NODE_TYPE(Assertion)
};

class HatAssertion : public Assertion {
 public:
  DECLARE_DERIVED_NODE_TYPE(HatAssertion)
};

class DollarAssertion : public Assertion {
 public:
  DECLARE_DERIVED_NODE_TYPE(DollarAssertion)
};

// \b or \B
class EscapedAssertion : public Assertion {
 public:
  explicit EscapedAssertion(bool uppercase) : uppercase_(uppercase) { }
  bool uppercase() const { return uppercase_; }
  DECLARE_DERIVED_NODE_TYPE(EscapedAssertion)
 private:
  bool uppercase_;
};

class DisjunctionAssertion : public Assertion {
 public:
  DisjunctionAssertion(Disjunction* disjunction, bool equal)
    : disjunction_(disjunction),
      equal_(equal) {}
  Disjunction* disjunction() const { return disjunction_; }
  bool equal() const { return equal_; }
  DECLARE_DERIVED_NODE_TYPE(DisjunctionAssertion)
 private:
  Disjunction* disjunction_;
  bool equal_;
};

class CharacterAtom : public Atom {
 public:
  explicit CharacterAtom(uint16_t ch) : character_(ch) { }
  uint16_t character() const { return character_; }
  DECLARE_DERIVED_NODE_TYPE(CharacterAtom)
 private:
  uint16_t character_;
};

class RangeAtom : public Atom {
 public:
  explicit RangeAtom(bool inverted, Ranges* ranges)
    : inverted_(inverted), ranges_(ranges) { }
  Ranges* ranges() const { return ranges_; }
  DECLARE_DERIVED_NODE_TYPE(RangeAtom)
 private:
  bool inverted_;
  Ranges* ranges_;
};

class DisjunctionAtom : public Atom {
 public:
  DisjunctionAtom(Disjunction* disjunction, bool captured)
    : disjunction_(disjunction),
      captured_(captured) { }
  Disjunction* disjunction() const { return disjunction_; }
  bool captured() const { return captured_; }
  DECLARE_DERIVED_NODE_TYPE(DisjunctionAtom)
 private:
  Disjunction* disjunction_;
  bool captured_;
};

#undef ACCEPT_VISITOR
#undef DECLARE_NODE_TYPE
#undef DECLARE_DERIVED_NODE_TYPE
#undef DECLARE_NODE_TYPE_BASE
} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_AST_H_
