#ifndef IV_LV5_AERO_PARSER_H_
#define IV_LV5_AERO_PARSER_H_
#include "ustringpiece.h"
#include "character.h"
#include "conversions.h"
#include "lv5/aero/range.h"
#include "lv5/aero/ast.h"
#include "lv5/aero/range_builder.h"
namespace iv {
namespace lv5 {
namespace aero {

#define IS(ch)\
  do {\
    if (c_ != ch) {\
      *e = 1;\
      return NULL;\
    }\
  } while (0)

#define EXPECT(ch)\
  do {\
    if (c_ != ch) {\
      *e = 1;\
      return NULL;\
    }\
    Advance();\
  } while (0)

#define UNEXPECT(ch)\
  do {\
    *e = 1;\
    return NULL;\
  } while (0)

#define RAISE(str)\
  do {\
    *e = 1;\
    return NULL;\
  } while (0)

#define CHECK  e);\
  if (*e) {\
    return NULL;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

class Parser {
 public:
  static const std::size_t kMaxPatternSize = core::Size::MB;
  static const int EOS = -1;
  Parser(core::Space* factory, const core::UStringPiece& source)
    : factory_(factory),
      ranges_(),
      source_(source),
      buffer8_(),
      pos_(0),
      end_(source.size()),
      c_(EOS) {
    Advance();
  }

  Expression* ParsePattern() {
    int error = 0;
    if (source_.size() > kMaxPatternSize) {
      return NULL;
    }
    Expression* expr = ParseDisjunction(EOS, &error);
    return (error) ? NULL : expr;
  }

 private:
  Expressions* NewExpressions() {
    return new (factory_->New(sizeof(Expressions)))
        Expressions(Expressions::allocator_type(factory_));
  }

  Alternatives* NewAlternatives() {
    return new (factory_->New(sizeof(Alternatives)))
        Alternatives(Alternatives::allocator_type(factory_));
  }

  template<typename T>
  Ranges* NewRange(const T& range) {
    return new (factory_->New(sizeof(Ranges)))
        Ranges(range.begin(), range.end(),
               Ranges::allocator_type(factory_));
  }

  Disjunction* ParseDisjunction(int end, int* e) {
    Alternatives* vec = NewAlternatives();
    Alternative* first = ParseAlternative(end, CHECK);
    vec->push_back(first);
    while (c_ == '|') {
      Advance();
      Alternative* alternative = ParseAlternative(end, CHECK);
      vec->push_back(alternative);
    }
    return new(factory_)Disjunction(vec);
  }

  Alternative* ParseAlternative(int end, int* e) {
    // Terms
    Expressions* vec = NewExpressions();
    Expression* target = NULL;
    while (c_ >= 0 && c_ != '|' && c_ != end) {
      bool atom = false;
      switch (c_) {
        case '^': {
          // Assertion
          target = new(factory_)HatAssertion();
          Advance();
          break;
        }

        case '$': {
          target = new(factory_)DollarAssertion();
          Advance();
          break;
        }

        case '(': {
          Advance();
          if (c_ == '?') {
            Advance();
            if (c_ == '=') {
              // ( ? = Disjunction )
              Advance();
              Disjunction* dis = ParseDisjunction(')', CHECK);
              EXPECT(')');
              target = new(factory_)DisjunctionAssertion(dis, true);
              atom = true;
            } else if (c_ == '!') {
              // ( ? ! Disjunction )
              Advance();
              Disjunction* dis = ParseDisjunction(')', CHECK);
              EXPECT(')');
              target = new(factory_)DisjunctionAssertion(dis, false);
              atom = true;
            } else if (c_ == ':') {  // (?:
              // ( ? : Disjunction )
              Advance();
              Disjunction* dis = ParseDisjunction(')', CHECK);
              EXPECT(')');
              target = new(factory_)DisjunctionAtom(dis, false);
              atom = true;
            } else {
              UNEXPECT(c_);
            }
          } else {
            // ( Disjunction )
            Disjunction* dis = ParseDisjunction(')', CHECK);
            EXPECT(')');
            target = new(factory_)DisjunctionAtom(dis, true);
            atom = true;
          }
          break;
        }

        case '.': {
          // Atom
          Advance();
          target = new(factory_)RangeAtom(
              false,
              NewRange(ranges_.GetEscapedRange('.')));
          atom = true;
          break;
        }

        case '\\': {
          Advance();
          if (c_ == 'b') {
            // \b
            Advance();
            target = new(factory_)EscapedAssertion(false);
          } else if (c_ == 'B') {
            // \B
            Advance();
            target = new(factory_)EscapedAssertion(true);
          } else {
            // AtomEscape
            target = ParseAtomEscape(CHECK);
            atom = true;
          }
          break;
        }

        case '[': {
          // CharacterClass
          target = ParseCharacterClass(CHECK);
          atom = true;
          break;
        }

        default: {
          // PatternCharacter
          if (!character::IsPatternCharacter(c_)) {
            UNEXPECT(c_);
          }
          target = new(factory_)CharacterAtom(c_);
          Advance();
          atom = true;
        }
      }
      if (atom && character::IsQuantifierPrefixStart(c_)) {
        target = ParseQuantifier(target, CHECK);
      }
      assert(target);
      vec->push_back(target);
    }
    return new(factory_)Alternative(vec);
  }

  Atom* ParseAtomEscape(int* e) {
    switch (c_) {
      case 'f': {
        Advance();
        return new(factory_)CharacterAtom('\f');
      }
      case 'n': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('n')));
      }
      case 'r': {
        Advance();
        return new(factory_)CharacterAtom('\r');
      }
      case 't': {
        Advance();
        return new(factory_)CharacterAtom('\t');
      }
      case 'v': {
        Advance();
        return new(factory_)CharacterAtom('\v');
      }
      case 'c': {
        // ControlLetter
        Advance();
        if (!core::character::IsASCIIAlpha(c_)) {
          UNEXPECT(c_);
        }
        Advance();
        return new(factory_)CharacterAtom('\\');
      }
      case 'x': {
        Advance();
        const uint16_t uc = ParseHexEscape(2, CHECK);
        return new(factory_)CharacterAtom(uc);
      }
      case 'u': {
        Advance();
        const uint16_t uc = ParseHexEscape(4, CHECK);
        return new(factory_)CharacterAtom(uc);
      }
      case core::character::code::ZWNJ: {
        Advance();
        return new(factory_)CharacterAtom(core::character::code::ZWNJ);
      }
      case core::character::code::ZWJ: {
        Advance();
        return new(factory_)CharacterAtom(core::character::code::ZWJ);
      }
      case 'd': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('d')));
      }
      case 'D': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('D')));
      }
      case 's': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('s')));
      }
      case 'S': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('S')));
      }
      case 'w': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('w')));
      }
      case 'W': {
        Advance();
        return new(factory_)RangeAtom(false,
                                      NewRange(ranges_.GetEscapedRange('W')));
      }
      default: {
        if (core::character::IsDecimalDigit(c_)) {
          const double numeric = ParseDecimalInteger(CHECK);
          const uint16_t uc = static_cast<uint16_t>(numeric);
          if (uc != numeric) {
            RAISE();
          }
          return new(factory_)CharacterAtom(uc);
        } else if (core::character::IsIdentifierPart(c_) || c_ < 0) {
          UNEXPECT(c_);
        } else {
          const uint16_t uc = c_;
          Advance();
          return new(factory_)CharacterAtom(uc);
        }
      }
    }
  }

  uint16_t ParseHexEscape(int len, int* e) {
    uint16_t res = 0;
    for (int i = 0; i < len; ++i) {
      const int d = core::HexValue(c_);
      if (d < 0) {
        for (int j = i - 1; j >= 0; --j) {
          PushBack();
        }
        *e = 1;  // error raise
        return 0;
      }
      res = res * 16 + d;
      Advance();
    }
    return res;
  }

  double ParseDecimalInteger(int* e) {
    assert(core::character::IsDecimalDigit(c_));
    buffer8_.clear();
    double result = 0.0;
    if (c_ != '0') {
      while (0 <= c_ && core::character::IsDecimalDigit(c_)) {
        buffer8_.push_back(c_);
        Advance();
      }
      result = core::ParseIntegerOverflow(
          buffer8_.data(),
          buffer8_.data() + buffer8_.size(), 10);
    }
    if (core::character::IsDecimalDigit(c_)) {
      *e = 1;
      return 0.0;
    }
    return result;
  }

  Expression* ParseCharacterClass(int* e) {
    assert(c_ == '[');
    Advance();
    ranges_.Clear();
    const bool invert = c_ == '^';
    if (invert) {
      Advance();
    }
    while (0 <= c_ && c_ != ']') {
      // first class atom
      assert(c_ != ']');
      uint16_t ranged1 = 0;
      const uint16_t start = ParseClassAtom(&ranged1, CHECK);
      if (c_ == '-') {
        // ClassAtom - ClassAtom ClassRanges
        Advance();
        if (c_ < 0) {
          UNEXPECT(c_);
        } else if (c_ == ']') {
          ranges_.AddOrEscaped(ranged1, start);
          ranges_.Add('-');
          break;
        } else {
          uint16_t ranged2 = 0;
          const uint16_t last = ParseClassAtom(&ranged2, CHECK);
          if (ranged1 || ranged2) {
            ranges_.AddOrEscaped(ranged1, start);
            ranges_.Add('-');
            ranges_.AddOrEscaped(ranged2, last);
          } else {
            if (!RangeBuilder::IsValidRange(start, last)) {
              RAISE();
            }
            ranges_.AddRange(start, last);
          }
        }
      } else {
        // ClassAtom
        // ClassAtom NonemptyClassRangesNoDash
        ranges_.AddOrEscaped(ranged1, start);
      }
    }
    EXPECT(']');
    return new(factory_)RangeAtom(invert, NewRange(ranges_.Finish()));
  }

  uint16_t ParseClassAtom(uint16_t* ranged, int* e) {
    if (c_ == '\\') {
      // ClassEscape
      // get range
      Advance();
      switch (c_) {
        case 'w':
        case 'W':
        case 'd':
        case 'D':
        case 's':
        case 'S': {
          *ranged = c_;
          Advance();
          return 0;
        }
        case 'f': {
          Advance();
          return '\f';
        }
        case 'n': {
          Advance();
          *ranged = c_;
          return '\n';
        }
        case 'r': {
          Advance();
          return '\r';
        }
        case 't': {
          Advance();
          return '\t';
        }
        case 'v': {
          Advance();
          return '\v';
        }
        case 'c': {
          // ControlLetter
          Advance();
          if (!core::character::IsASCIIAlpha(c_)) {
            *e = 1;
            return 0;
          }
          Advance();
          return '\\';
        }
        case 'x': {
          Advance();
          return ParseHexEscape(2, e);
        }
        case 'u': {
          Advance();
          return ParseHexEscape(4, e);
        }
        case core::character::code::ZWNJ: {
          Advance();
          return core::character::code::ZWNJ;
        }
        case core::character::code::ZWJ: {
          Advance();
          return core::character::code::ZWJ;
        }
        default: {
          if (core::character::IsDecimalDigit(c_)) {
            const double numeric = ParseDecimalInteger(CHECK);
            const uint16_t uc = static_cast<uint16_t>(numeric);
            if (uc != numeric) {
              RAISE();
            }
            return uc;
          } else if (core::character::IsIdentifierPart(c_) || c_ < 0) {
            *e = 1;
            return 0;
          } else {
            const uint16_t ch = c_;
            Advance();
            return ch;
          }
        }
      }
    } else {
      const uint16_t ch = c_;
      Advance();
      return ch;
    }
  }

  Quantifiered* ParseQuantifier(Expression* target, int* e) {
    // prefix
    int min = 0, max = 0;
    switch (c_) {
      case '*': {
        Advance();
        min = 0;
        max = kRegExpInfinity;
        break;
      }
      case '+': {
        Advance();
        min = 1;
        max = kRegExpInfinity;
        break;
      }
      case '?': {
        Advance();
        min = 0;
        max = 1;
        break;
      }
      case '{': {
        Advance();
        const double numeric1 = ParseDecimalInteger(CHECK);
        if (numeric1 > kRegExpInfinity) {
          min = kRegExpInfinity;
        } else {
          min = static_cast<int>(numeric1);
          if (min != numeric1) {
            RAISE();
          }
        }
        if (c_ == ',') {
          Advance();
          if (c_ == '}') {
            max = kRegExpInfinity;
          } else {
            const double numeric2 = ParseDecimalInteger(CHECK);
            if (numeric2 > kRegExpInfinity) {
              max = kRegExpInfinity;
            } else {
              max = static_cast<int>(numeric2);
              if (max != numeric2) {
                RAISE();
              }
            }
          }
        } else if (c_ == '}') {
          max = min;
        }
        EXPECT('}');
        break;
      }
      default: {
        UNEXPECT(c_);
      }
    }
    if (max < min) {
      RAISE();
    }
    // postfix
    bool greedy = true;
    if (c_ == '?') {
      Advance();
      greedy = false;
    }
    return new(factory_)Quantifiered(target, min, max, greedy);
  }

  inline void Advance() {
    if (pos_ == end_) {
      c_ = EOS;
    } else {
      c_ = source_[pos_++];
    }
  }

  void PushBack() {
    if (pos_ < 2) {
      c_ = EOS;
    } else {
      c_ = source_[pos_-2];
      --pos_;
    }
  }

  core::Space* factory_;
  RangeBuilder ranges_;
  const core::UStringPiece source_;
  std::vector<char> buffer8_;
  std::size_t pos_;
  const std::size_t end_;
  int c_;
};

#undef IS
#undef EXPECT
#undef UNEXPECT
#undef RAISE
#undef CHECK
} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_PARSER_H_
