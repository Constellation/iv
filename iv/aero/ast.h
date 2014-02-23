#ifndef IV_AERO_AST_H_
#define IV_AERO_AST_H_
#include <limits>
#include <iv/space.h>
#include <iv/aero/visitor.h>
#include <iv/aero/range.h>
namespace iv {
namespace aero {

static const int32_t kRegExpInfinity = std::numeric_limits<int32_t>::max();

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
  inline virtual const type* As##type() const { return nullptr; }\
  inline virtual type* As##type() { return nullptr; }

class Expression : public core::SpaceObject {
 public:
  virtual ~Expression() { }
  AERO_EXPRESSION_AST_NODES(DECLARE_NODE_TYPE_BASE)
  virtual void Accept(Visitor* visitor) = 0;
};

inline void Expression::Accept(Visitor* visitor) { }

typedef core::SpaceVector<core::Space, Expression*> Expressions;
typedef core::SpaceVector<core::Space, Alternative*> Alternatives;
typedef core::SpaceVector<core::Space, Range> Ranges;

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
  Quantifiered(Expression* expr, int32_t min, int32_t max, bool greedy)
    : expression_(expr), min_(min), max_(max), greedy_(greedy) { }
  Expression* expression() const { return expression_; }
  int32_t min() const { return min_; }
  int32_t max() const { return max_; }
  bool greedy() const { return greedy_; }
  DECLARE_DERIVED_NODE_TYPE(Quantifiered)
 private:
  Expression* expression_;
  int32_t min_;
  int32_t max_;
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
  DisjunctionAssertion(Disjunction* disjunction, bool inverted)
    : disjunction_(disjunction),
      inverted_(inverted) {}
  Disjunction* disjunction() const { return disjunction_; }
  bool inverted() const { return inverted_; }
  DECLARE_DERIVED_NODE_TYPE(DisjunctionAssertion)
 private:
  Disjunction* disjunction_;
  bool inverted_;
};

class BackReferenceAtom : public Atom {
 public:
  explicit BackReferenceAtom(uint16_t ref, uint16_t octal)
    : reference_(ref), octal_(octal) { }
  uint16_t reference() const { return reference_; }
  uint16_t octal() const { return octal_; }
  DECLARE_DERIVED_NODE_TYPE(BackReferenceAtom)
 private:
  uint16_t reference_;
  uint16_t octal_;
};

class CharacterAtom : public Atom {
 public:
  explicit CharacterAtom(char16_t ch) : character_(ch) { }
  char16_t character() const { return character_; }
  DECLARE_DERIVED_NODE_TYPE(CharacterAtom)
 private:
  char16_t character_;
};

class RangeAtom : public Atom {
 public:
  explicit RangeAtom(bool inverted, bool singles,
                     uint32_t counts, Ranges* ranges)
    : inverted_(inverted)
    , singles_(singles)
    , counts_(counts)
    , ranges_(ranges) { }
  const Ranges& ranges() const { return *ranges_; }
  template<typename Iter>
  Iter FillBuffer(Iter it) const {
    for (const auto& range : ranges()) {
      for (auto current = range.first; current <= range.second; ++current) {
        *it++ = current;
      }
    }
    return it;
  }
  bool inverted() const { return inverted_; }
  bool singles() const { return singles_; }
  uint32_t counts() const { return counts_; }
  DECLARE_DERIVED_NODE_TYPE(RangeAtom)
 private:
  bool inverted_;
  bool singles_;
  uint32_t counts_;
  Ranges* ranges_;
};

class DisjunctionAtom : public Atom {
 public:
  explicit DisjunctionAtom(Disjunction* disjunction)
    : disjunction_(disjunction),
      captured_(false), num_(0) { }
  DisjunctionAtom(Disjunction* disjunction, uint32_t num)
    : disjunction_(disjunction),
      captured_(true), num_(num) { }
  Disjunction* disjunction() const { return disjunction_; }
  bool captured() const { return captured_; }
  uint32_t num() const { return num_; }
  DECLARE_DERIVED_NODE_TYPE(DisjunctionAtom)
 private:
  Disjunction* disjunction_;
  bool captured_;
  uint32_t num_;
};

#undef ACCEPT_VISITOR
#undef DECLARE_NODE_TYPE
#undef DECLARE_DERIVED_NODE_TYPE
#undef DECLARE_NODE_TYPE_BASE
} }  // namespace iv::aero
#endif  // IV_AERO_AST_H_
