#ifndef IV_LV5_AERO_COMPILER_H_
#define IV_LV5_AERO_COMPILER_H_
#include <vector>
#include "detail/cstdint.h"
#include "detail/unordered_set.h"
#include "noncopyable.h"
#include "character.h"
#include "debug.h"
#include "lv5/aero/flags.h"
#include "lv5/aero/op.h"
#include "lv5/aero/code.h"
#include "lv5/aero/captures.h"
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
      max_captures_(),
      current_captures_num_(),
      jmp_(),
      counters_(),
      counters_size_(0) {
  }

  Code Compile(const ParsedData& data) {
    max_captures_ = data.max_captures();
    current_captures_num_ = 0;
    EmitQuickCheck(data.pattern());
    data.pattern()->Accept(this);
    Emit<OP::SUCCESS>();
    return Code(code_, max_captures_, counters_size_);
  }

  uint32_t counters_size() const { return counters_size_; }
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
      const CounterHolder holder(this);
      Emit<OP::STORE_SP>();
      Emit4(holder.counter());
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos1 = Current();
      Emit4(0);  // dummy
      Visit(assertion->disjunction());
      Emit<OP::ASSERTION_FAILURE>();
      Emit4(holder.counter());
      Emit4At(pos1, Current());
    } else {
      const CounterHolder holder(this);
      Emit<OP::STORE_SP>();
      Emit4(holder.counter());
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos1 = Current();
      Emit4(0);  // dummy
      Visit(assertion->disjunction());
      Emit<OP::ASSERTION_SUCCESS>();
      Emit4(holder.counter());
      const std::size_t pos2 = Current();
      Emit4(0);  // dummy
      Emit4At(pos1, Current());
      Emit<OP::FAILURE>();
      Emit4At(pos2, Current());
    }
  }

  void Visit(BackReferenceAtom* atom) {
    if (IsIgnoreCase()) {
      Emit<OP::BACK_REFERENCE_IGNORE_CASE>();
    } else {
      Emit<OP::BACK_REFERENCE>();
    }
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
      current_captures_num_ = std::max(current_captures_num_, atom->num());
      Emit<OP::START_CAPTURE>();
      Emit4(atom->num());
      Visit(dis);
      Emit<OP::END_CAPTURE>();
      Emit4(atom->num());
    } else {
      Visit(dis);
    }
  }

  void Visit(Quantifiered* atom) {
    const uint32_t now = current_captures_num_;
    const uint32_t capture =
        GainNextCaptures(atom->expression(), current_captures_num_);

    // optimized paths
    // some frequency patterns like *, ? can be implemented without counters
    if (atom->max() == 0) {
      // no term
      EmitClearCaptures(capture);
    } else if (atom->min() == atom->max()) {
      // same quantity pattern. greedy parameter is no effect.
      const CounterHolder holder(this);
      EmitFixed(atom, holder.counter(), atom->min(), capture);
    } else if (atom->min() == 0) {
      if (atom->max() == 1) {
        // ? pattern. counter not used
        if (atom->greedy()) {
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos = Current();
          Emit4(0u);  // dummy
          EmitClearCaptures(capture);
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
          EmitClearCaptures(capture);
          atom->expression()->Accept(this);
          Emit4At(pos2, Current());
        }
      } else if (atom->max() == kRegExpInfinity) {
        // * pattern. counter not used
        if (atom->greedy()) {
          const CounterHolder pos_holder(this);
          const std::size_t pos1 = Current();
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos2 = Current();
          Emit4(0u);  // dummy
          EmitClearCaptures(capture);
          Emit<OP::STORE_POSITION>();
          Emit4(pos_holder.counter());
          atom->expression()->Accept(this);
          Emit<OP::POSITION_TEST>();
          Emit4(pos_holder.counter());
          Emit<OP::JUMP>();
          Emit4(pos1);
          Emit4At(pos2, Current());
        } else {
          const CounterHolder pos_holder(this);
          const std::size_t pos1 = Current();
          Emit<OP::PUSH_BACKTRACK>();
          const std::size_t pos2 = Current();
          Emit4(0u);  // dummy 1
          Emit<OP::JUMP>();
          const std::size_t pos3 = Current();
          Emit4(0u);  // dummy 2
          Emit4At(pos2, Current());
          EmitClearCaptures(capture);
          Emit<OP::STORE_POSITION>();
          Emit4(pos_holder.counter());
          atom->expression()->Accept(this);
          Emit<OP::POSITION_TEST>();
          Emit4(pos_holder.counter());
          Emit<OP::JUMP>();
          Emit4(pos1);
          Emit4At(pos3, Current());
        }
      } else {
        // min == 0 and max
        const CounterHolder holder(this);
        const CounterHolder pos_holder(this);
        EmitFollowingRepeat(atom,
                            holder.counter(),
                            atom->max(), capture, pos_holder.counter());
      }
    } else {
      const CounterHolder holder(this);
      const CounterHolder pos_holder(this);
      assert(atom->min() != 0);
      EmitFixed(atom, holder.counter(), atom->min(), capture);
      current_captures_num_ = now;
      const uint32_t delta = atom->max() - atom->min();
      assert(delta > 0);
      EmitFollowingRepeat(atom,
                          holder.counter(),
                          delta, capture, pos_holder.counter());
    }
    current_captures_num_ = capture;
  }

  void EmitFixed(Quantifiered* atom,
                 uint32_t counter, int32_t fixed, uint32_t capture) {
    // COUNTER_ZERO | COUNTER_TARGET
    Emit<OP::COUNTER_ZERO>();
    Emit4(counter);
    const std::size_t target = Current();
    EmitClearCaptures(capture);
    atom->expression()->Accept(this);
    // COUNTER_NEXT | COUNTER_TARGET | MAX | JUMP_TARGET
    Emit<OP::COUNTER_NEXT>();
    Emit4(counter);
    Emit4(fixed);
    Emit4(target);
  }

  void EmitFollowingRepeat(Quantifiered* atom,
                           uint32_t counter, int32_t max,
                           uint32_t capture, uint32_t position_test) {
    Emit<OP::COUNTER_ZERO>();
    Emit4(counter);
    if (atom->greedy()) {
      const std::size_t pos1 = Current();
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos2 = Current();
      Emit4(0u);
      EmitClearCaptures(capture);
      Emit<OP::STORE_POSITION>();
      Emit4(position_test);
      atom->expression()->Accept(this);
      Emit<OP::POSITION_TEST>();
      Emit4(position_test);
      Emit<OP::COUNTER_NEXT>();
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
      EmitClearCaptures(capture);
      Emit<OP::STORE_POSITION>();
      Emit4(position_test);
      atom->expression()->Accept(this);
      Emit<OP::POSITION_TEST>();
      Emit4(position_test);
      Emit<OP::COUNTER_NEXT>();
      Emit4(counter);
      Emit4(max);
      Emit4(pos1);
      Emit4At(pos3, Current());
    }
  }

  void EmitClearCaptures(uint32_t capture) {
    if (current_captures_num_ != capture) {
      assert(current_captures_num_ < capture);
      Emit<OP::CLEAR_CAPTURES>();
      Emit4(current_captures_num_ + 1);
      Emit4(capture + 1);
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

  void Emit1At(std::size_t at, uint8_t val) {
    code_[at] = val;
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
  uint32_t max_captures_;
  uint32_t current_captures_num_;
  std::vector<std::size_t> jmp_;
  std::unordered_set<uint32_t> counters_;
  uint32_t counters_size_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_COMPILER_H_
