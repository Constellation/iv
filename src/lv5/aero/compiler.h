#ifndef IV_LV5_AERO_COMPILER_H_
#define IV_LV5_AERO_COMPILER_H_
#include <vector>
#include <iostream>
#include "detail/cstdint.h"
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
  Compiler(int flags)
    : flags_(flags),
      code_(),
      captures_(),
      jmp_() {
  }

  const std::vector<uint8_t>& Compile(Disjunction* expr) {
    captures_.push_back(expr);
    expr->Accept(this);
    Emit<OP::MATCH>();
    return code_;
  }

  bool IsIgnoreCase() const { return flags_ & IGNORE_CASE; }
  bool IsMultiline() const { return flags_ & MULTILINE; }

 private:
  void Visit(Disjunction* dis) {
    assert(!dis->alternatives().empty());
    const std::size_t jmp = jmp_.size();
    for (std::size_t i = 0,
         len = dis->alternatives().size() - 1; i < len; ++i) {
      Emit<OP::PUSH_CURRENT_POSITION>();
      Emit<OP::PUSH_BACKTRACK>();
      const std::size_t pos = Current();
      Emit4(0);
      Visit(dis->alternatives()[i]);
      Emit<OP::JUMP>();
      jmp_.push_back(Current());
      Emit4(0);
      Emit4At(pos, Current());  // backtrack
      Emit<OP::POP_CURRENT_POSITION>();
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
    Emit<OP::ASSERTION_BOL>();
  }

  void Visit(DollarAssertion* assertion) {
    Emit<OP::ASSERTION_EOL>();
  }

  void Visit(EscapedAssertion* assertion) {
    if (assertion->uppercase()) {
      Emit<OP::ASSERTION_WORD_BOUNDARY_INVERTED>();
    } else {
      Emit<OP::ASSERTION_WORD_BOUNDARY>();
    }
  }

  void Visit(DisjunctionAssertion* assertion) {
  }

  void Visit(CharacterAtom* atom) {
    const uint16_t ch = atom->character();
    if (IsIgnoreCase()) {
      const uint16_t uu = core::character::ToUpperCase(ch);
      const uint16_t lu = core::character::ToLowerCase(ch);
      if (uu != lu) {
        Emit<OP::CHECK_2CHAR_OR>();
        Emit2(uu);
        Emit2(lu);
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
    atom->expression()->Accept(this);
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

  int flags_;
  std::vector<uint8_t> code_;
  std::vector<Disjunction*> captures_;
  std::vector<std::size_t> jmp_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_COMPILER_H_
