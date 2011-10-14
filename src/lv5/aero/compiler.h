#ifndef IV_LV5_AERO_COMPILER_H_
#define IV_LV5_AERO_COMPILER_H_
#include <vector>
#include <iostream>
#include "detail/cstdint.h"
#include "detail/unordered_set.h"
#include "noncopyable.h"
#include "character.h"
#include "debug.h"
#include "lv5/aero/flags.h"
#include "lv5/aero/op.h"
namespace iv {
namespace lv5 {
namespace aero {

class Compiler : private Visitor {
 public:
  class CounterHolder {
   public:
    explicit CounterHolder(Compiler* compiler)
      : compiler_(compiler), counter_(compiler->AcquireCounter()) { }
    ~CounterHolder() { compiler_->ReleaseCounter(counter_); }
    uint32_t counter() const { return counter_; }
   private:
    Compiler* compiler_;
    uint32_t counter_;
  };

  explicit Compiler(int flags)
    : flags_(flags),
      code_(),
      captures_(),
      jmp_(),
      counters_(),
      counters_size_(0) {
  }

  const std::vector<uint8_t>& Compile(Disjunction* expr) {
    EmitQuickCheck(expr);
    captures_.push_back(expr);
    expr->Accept(this);
    Emit<OP::SUCCESS>();
    return code_;
  }

  uint32_t counters_size() const { return counters_size_; }
  uint32_t captures() const { return captures_.size(); }

  bool IsIgnoreCase() const { return flags_ & IGNORE_CASE; }
  bool IsMultiline() const { return flags_ & MULTILINE; }

 private:
  void EmitQuickCheck(Disjunction* dis) {
    // emit quick check phase for this pattern.
    // when RegExp /test/ is provided, we emit code that searching 't'
    // and if it is accepted, goto main body with this character position.
  }

  void Visit(Disjunction* dis) {
    assert(!dis->alternatives().empty());
    const std::size_t jmp = jmp_.size();
    for (std::size_t i = 0,
         len = dis->alternatives().size() - 1; i < len; ++i) {
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos = Current();
      Emit4(0);  // dummy
      Visit(dis->alternatives()[i]);
      Emit<OP::JUMP>();
      jmp_.push_back(Current());
      Emit4(0);
      Emit4At(pos, Current());  // backtrack
    }
    Alternative* last = dis->alternatives().back();
    Visit(last);
    for (std::vector<std::size_t>::const_iterator it = jmp_.begin() + jmp,
         last = jmp_.end(); it != last; ++it) {
      Emit4At(*it, Current());
    }
    jmp_.resize(jmp);  // shrink jmp_ target buffer
  }

  void Visit(Alternative* alt) {
    for (Expressions::const_iterator it = alt->terms().begin(),
         last = alt->terms().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }

  void Visit(HatAssertion* assertion) {
    if (IsMultiline()) {
      Emit<OP::ASSERTION_BOB>();
    } else {
      Emit<OP::ASSERTION_BOL>();
    }
  }

  void Visit(DollarAssertion* assertion) {
    if (IsMultiline()) {
      Emit<OP::ASSERTION_EOB>();
    } else {
      Emit<OP::ASSERTION_EOL>();
    }
  }

  void Visit(EscapedAssertion* assertion) {
    if (assertion->uppercase()) {
      Emit<OP::ASSERTION_WORD_BOUNDARY_INVERTED>();
    } else {
      Emit<OP::ASSERTION_WORD_BOUNDARY>();
    }
  }

  void Visit(DisjunctionAssertion* assertion) {
    if (assertion->inverted()) {
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos = Current();
      Emit4(0);  // dummy
      Visit(assertion->disjunction());
      Emit<OP::DISCARD_BACKTRACK>();
      Emit<OP::FAILURE>();
      Emit4At(pos, Current());  // backtrack
    } else {
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos1 = Current();
      Emit4(0);  // dummy
      Visit(assertion->disjunction());
      Emit<OP::ASSERTION>();
      const std::size_t pos2 = Current();
      Emit4(0);  // dummy
      Emit4At(pos1, Current());  // backtrack
      Emit<OP::FAILURE>();
      Emit4At(pos2, Current());
    }
  }

  void Visit(BackReferenceAtom* atom) {
    Emit<OP::BACK_REFERENCE>();
    Emit2(atom->reference());
  }

  void Visit(CharacterAtom* atom) {
    const uint16_t ch = atom->character();
    if (IsIgnoreCase()) {
      const uint16_t uu = core::character::ToUpperCase(ch);
      const uint16_t lu = core::character::ToLowerCase(ch);
      if (!(uu == lu && uu == ch)) {
        if (uu == ch || lu == ch) {
          Emit<OP::CHECK_2CHAR_OR>();
          Emit2(uu);
          Emit2(lu);
        } else {
          Emit<OP::CHECK_3CHAR_OR>();
          Emit2(ch);
          Emit2(uu);
          Emit2(lu);
        }
        return;
      }
    }
    if (ch > 0xFF) {
      Emit<OP::CHECK_2BYTE_CHAR>();
      Emit2(ch);
    } else {
      Emit<OP::CHECK_1BYTE_CHAR>();
      Emit1(ch);
    }
  }

  void Visit(RangeAtom* atom) {
    const uint32_t len = atom->ranges().size() * 4;
    if (atom->inverted()) {
      Emit<OP::CHECK_RANGE_INVERTED>();
    } else {
      Emit<OP::CHECK_RANGE>();
    }
    Emit4(len);
    for (Ranges::const_iterator it = atom->ranges().begin(),
         last = atom->ranges().end(); it != last; ++it) {
      Emit2(it->first);
      Emit2(it->second);
    }
  }

  void Visit(DisjunctionAtom* atom) {
    Disjunction* dis = atom->disjunction();
    if (atom->captured()) {
      captures_.push_back(dis);
    }
    Visit(dis);
  }

  void Visit(Quantifiered* atom) {
    if (atom->max() == 0) {
      // no term
      return;
    }

    // optimized paths
    // some frequency patterns like *, ? can be implemented without counters
    if (atom->min() == atom->max()) {
      // same quantity pattern. greedy parameter is no effect.
      const CounterHolder holder(this);
      EmitFixed(atom, holder.counter(), atom->min());
    } else if (atom->min() == 0) {
      if (atom->max() == 1) {
        // ? pattern. counter not used
        if (atom->greedy()) {
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos = Current();
          Emit4(0u);  // dummy
          atom->expression()->Accept(this);
          Emit4At(pos, Current());
        } else {
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos1 = Current();
          Emit4(0u);  // dummy 1
          Emit<OP::JUMP>();
          const std::size_t pos2 = Current();
          Emit4(0u);  // dummy 2
          Emit4At(pos1, Current());
          atom->expression()->Accept(this);
          Emit4At(pos2, Current());
        }
      } else if (atom->max() == kRegExpInfinity) {
        // * pattern. counter not used
        if (atom->greedy()) {
          const std::size_t pos1 = Current();
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos2 = Current();
          Emit4(0u);  // dummy
          atom->expression()->Accept(this);
          Emit<OP::JUMP>();
          Emit4(pos1);
          Emit4At(pos2, Current());
        } else {
          const std::size_t pos1 = Current();
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos2 = Current();
          Emit4(0u);  // dummy 1
          Emit<OP::JUMP>();
          const std::size_t pos3 = Current();
          Emit4(0u);  // dummy 2
          Emit4At(pos2, Current());
          atom->expression()->Accept(this);
          Emit<OP::JUMP>();
          Emit4(pos1);
          Emit4At(pos3, Current());
        }
      } else {
        // min == 0 and max
        const CounterHolder holder(this);
        EmitFollowingRepeat(atom, holder.counter(), atom->max());
      }
    } else {
      const CounterHolder holder(this);
      assert(atom->min() != 0);
      EmitFixed(atom, holder.counter(), atom->min());
      const uint32_t delta = atom->max() - atom->min();
      assert(delta > 0);
      EmitFollowingRepeat(atom, holder.counter(), delta);
    }
  }

  void EmitFixed(Quantifiered* atom, uint32_t counter, uint32_t fixed) {
    // COUNTER_ZERO | COUNTER_TARGET
    Emit<OP::COUNTER_ZERO>();
    Emit4(counter);
    const std::size_t target = Current();
    atom->expression()->Accept(this);
    // COUNTER_NEXT | COUNTER_TARGET | MAX | JUMP_TARGET
    Emit<OP::COUNTER_NEXT>();
    Emit4(counter);
    Emit4(fixed);
    Emit4(target);
  }

  void EmitFollowingRepeat(Quantifiered* atom, uint32_t counter, uint32_t max) {
    Emit<OP::COUNTER_ZERO>();
    Emit4(counter);
    if (atom->greedy()) {
      const std::size_t pos1 = Current();
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos2 = Current();
      Emit4(0u);
      atom->expression()->Accept(this);
      Emit<OP::COUNTER_GUARD>();
      Emit4(counter);
      Emit4(max);
      Emit4(pos1);
      Emit4At(pos2, Current());
    } else {
      const std::size_t pos1 = Current();
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos2 = Current();
      Emit4(0u);
      Emit<OP::JUMP>();
      const std::size_t pos3 = Current();
      Emit4(0u);
      Emit4At(pos2, Current());
      atom->expression()->Accept(this);
      Emit<OP::COUNTER_GUARD>();
      Emit4(counter);
      Emit4(max);
      Emit4(pos1);
      Emit4At(pos3, Current());
    }
  }

  template<OP::Type op>
  void Emit() {
    code_.push_back(op);
  }

  void Emit1(uint32_t val) {
    code_.push_back(val & 0xFF);
  }

  void Emit2(uint32_t val) {
    code_.push_back((val >> 8) & 0xFF);
    code_.push_back(val & 0xFF);
  }

  void Emit4(uint32_t val) {
    code_.push_back((val >> 24) & 0xFF);
    code_.push_back((val >> 16) & 0xFF);
    code_.push_back((val >> 8) & 0xFF);
    code_.push_back(val & 0xFF);
  }

  void Emit4At(std::size_t at, uint32_t val) {
    code_[at] = (val >> 24) & 0xFF;
    code_[at + 1] = (val >> 16) & 0xFF;
    code_[at + 2] = (val >> 8) & 0xFF;
    code_[at + 3] = val & 0xFF;
  }

  std::size_t Current() const {
    return code_.size();
  }

  void ReleaseCounter(uint32_t val) {
    counters_.insert(val);
  }

  uint32_t AcquireCounter() {
    std::unordered_set<uint32_t>::const_iterator it = counters_.begin();
    if (it == counters_.end()) {
      return counters_size_++;
    } else {
      const uint32_t res = *it;
      counters_.erase(it);
      return res;
    }
  }

  int flags_;
  std::vector<uint8_t> code_;
  std::vector<Disjunction*> captures_;
  std::vector<std::size_t> jmp_;
  std::unordered_set<uint32_t> counters_;
  uint32_t counters_size_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_COMPILER_H_
