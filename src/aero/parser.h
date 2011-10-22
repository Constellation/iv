#ifndef IV_AERO_PARSER_H_
#define IV_AERO_PARSER_H_
#include "detail/unordered_set.h"
#include "ustringpiece.h"
#include "character.h"
#include "conversions.h"
#include "space.h"
#include "aero/range.h"
#include "aero/flags.h"
#include "aero/ast.h"
#include "aero/range_builder.h"
namespace iv {
namespace aero {

#define EXPECT(ch)\
  do {\
    if (c_ != ch) {\
      *e = UNEXPECTED_CHARACTER;\
      return NULL;\
    }\
    Advance();\
  } while (0)

#define RAISE(code)\
  do {\
    *e = code;\
    return NULL;\
  } while (0)

#define UNEXPECT(ch) RAISE(UNEXPECTED_CHARACTER)

#define CHECK  e);\
  if (*e) {\
    return NULL;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

class ParsedData {
 public:
  ParsedData(Disjunction* dis, uint32_t max_captures,
             const core::UStringPiece& source)
    : pattern_(dis), max_captures_(max_captures), source_(source) { }
  Disjunction* pattern() const { return pattern_; }
  uint32_t max_captures() const { return max_captures_; }
  const core::UStringPiece& source() const { return source_; }
 private:
  Disjunction* pattern_;
  uint32_t max_captures_;
  core::UStringPiece source_;
};

class Parser {
 public:
  static const std::size_t kMaxPatternSize = core::Size::MB;
  static const int EOS = -1;
  enum ErrorCode {
    NONE = 0,
    UNEXPECTED_CHARACTER = 1,
    NUMBER_TOO_BIG = 2,
    INVALID_RANGE = 3,
    INVALID_QUANTIFIER = 4,
    TOO_LONG_REGEXP = 5
  };

  Parser(core::Space* factory, const core::UStringPiece& source, int flags)
    : flags_(flags),
      factory_(factory),
      ranges_(IsIgnoreCase()),
      source_(source),
      buffer8_(),
      pos_(0),
      end_(source.size()),
      c_(EOS),
      captures_(1) {
    Advance();
  }

  ParsedData ParsePattern(int* e) {
    if (source_.size() > kMaxPatternSize) {
      *e = TOO_LONG_REGEXP;
      return ParsedData(NULL, captures_, source_);
    }
    Disjunction* dis = ParseDisjunction<EOS>(e);
    if (c_ != EOS) {
      *e = UNEXPECTED_CHARACTER;
      return ParsedData(NULL, captures_, source_);
    }
    return ParsedData(dis, captures_, source_);
  }

 private:
  bool IsIgnoreCase() const { return flags_ & IGNORE_CASE; }
  bool IsMultiline() const { return flags_ & MULTILINE; }

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

  template<int END>
  Disjunction* ParseDisjunction(int* e) {
    Alternatives* vec = NewAlternatives();
    Alternative* first = ParseAlternative<END>(CHECK);
    vec->push_back(first);
    while (c_ == '|') {
      Advance();
      Alternative* alternative = ParseAlternative<END>(CHECK);
      vec->push_back(alternative);
    }
    return new(factory_)Disjunction(vec);
  }

  template<int END>
  Alternative* ParseAlternative(int* e) {
    // Terms
    Expressions* vec = NewExpressions();
    Expression* target = NULL;
    while (c_ >= 0 && c_ != '|' && c_ != END) {
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
              Disjunction* dis = ParseDisjunction<')'>(CHECK);
              EXPECT(')');
              target = new(factory_)DisjunctionAssertion(dis, false);
              atom = true;
            } else if (c_ == '!') {
              // ( ? ! Disjunction )
              Advance();
              Disjunction* dis = ParseDisjunction<')'>(CHECK);
              EXPECT(')');
              target = new(factory_)DisjunctionAssertion(dis, true);
              atom = true;
            } else if (c_ == ':') {  // (?:
              // ( ? : Disjunction )
              Advance();
              Disjunction* dis = ParseDisjunction<')'>(CHECK);
              EXPECT(')');
              target = new(factory_)DisjunctionAtom(dis);
              atom = true;
            } else {
              UNEXPECT(c_);
            }
          } else {
            const uint32_t num = captures_++;
            // ( Disjunction )
            Disjunction* dis = ParseDisjunction<')'>(CHECK);
            EXPECT(')');
            target = new(factory_)DisjunctionAtom(dis, num);
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
          //
          // and add special cases like /]/
          // see IE Blog
          if (!character::IsPatternCharacter(c_) && (c_ != ']')) {
            UNEXPECT(c_);
          }
          // AtomNoBrace or Atom
          if (character::IsBrace(c_)) {
            // check this is not Quantifier format
            // if Quantifier format, reject it.
            if (c_ == '{') {
              bool ok = false;
              ParseQuantifier(NULL, &ok, CHECK);
              if (ok) {
                UNEXPECT('{');
              }
            }
          } else {
            atom = true;
          }
          target = new(factory_)CharacterAtom(c_);
          Advance();
        }
      }
      if (atom && c_ >= 0 && character::IsQuantifierPrefixStart(c_)) {
        bool ok = false;
        target = ParseQuantifier(target, &ok, CHECK);
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
          return new(factory_)CharacterAtom('c');
        }
        const uint16_t ch = c_;
        Advance();
        return new(factory_)CharacterAtom(ch % 32);
      }
      case 'x': {
        Advance();
        const int uc = ParseHexEscape(2);
        return new(factory_)CharacterAtom((uc == -1) ? 'x' : uc);
      }
      case 'u': {
        Advance();
        const int uc = ParseHexEscape(4);
        return new(factory_)CharacterAtom((uc == -1) ? 'u' : uc);
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
      case '0': {
        // maybe octal
        const double numeric = ParseNumericClassEscape();
        const uint16_t ch = static_cast<uint16_t>(numeric);
        if (ch != numeric) {
          RAISE(NUMBER_TOO_BIG);
        }
        return new(factory_)CharacterAtom(ch);
      }
      default: {
        if ('1' <= c_ && c_ <= '9') {
          return ParseBackReference(e);
        } else if (c_ < 0) {
          UNEXPECT(c_);
        } else {
          const uint16_t uc = c_;
          Advance();
          return new(factory_)CharacterAtom(uc);
        }
      }
    }
  }

  int ParseHexEscape(int len) {
    uint16_t res = 0;
    for (int i = 0; i < len; ++i) {
      const int d = core::HexValue(c_);
      if (d < 0) {
        for (int j = i - 1; j >= 0; --j) {
          PushBack();
        }
        return -1;
      }
      res = res * 16 + d;
      Advance();
    }
    return res;
  }

  Atom* ParseBackReference(int* e) {
    // not accept \0 as reference
    // ParseBackReference returns octal value and decimal value
    // if octal value is invalid, zero is assigned.
    assert(core::character::IsDecimalDigit(c_));
    assert(c_ != '0');
    buffer8_.clear();
    bool octal_value_candidate = true;
    while (0 <= c_ && core::character::IsDecimalDigit(c_)) {
      if (!core::character::IsOctalDigit(c_)) {
        octal_value_candidate = false;
      }
      buffer8_.push_back(c_);
      Advance();
    }
    const double decimal =
        core::ParseIntegerOverflow(
            buffer8_.data(),
            buffer8_.data() + buffer8_.size(), 10);
    const double octal = (octal_value_candidate) ?
        core::ParseIntegerOverflow(
            buffer8_.data(),
            buffer8_.data() + buffer8_.size(), 8) : 0;
    const uint16_t ref = static_cast<uint16_t>(decimal);
    if (ref != decimal) {
      RAISE(NUMBER_TOO_BIG);
    }
    // back reference validation is not done
    // so, we should validate there is reference
    // which back reference is targeting
    return new(factory_)BackReferenceAtom(ref, static_cast<uint16_t>(octal));
  }


  double ParseNumericClassEscape() {
    assert(core::character::IsDecimalDigit(c_));
    const uint16_t ch = c_;
    buffer8_.clear();

    bool start_with_zero = false;
    if (c_ == '0') {
      start_with_zero = true;
      Advance();
      if (0 > c_ || !core::character::IsOctalDigit(c_)) {
        return 0;
      }
    }

    const std::size_t pos = pos_;
    const bool octal_candidate = core::character::IsOctalDigit(c_);
    while (true) {
      if (c_ < '0' || '7' < c_) {
        break;
      }
      buffer8_.push_back(c_);
      Advance();
    }

    // octal digit like \07a
    if (octal_candidate && (c_ < 0 || !core::character::IsDecimalDigit(c_))) {
      return core::ParseIntegerOverflow(
          buffer8_.data(),
          buffer8_.data() + buffer8_.size(), 8);
    }

    // like \019
    // divide \0 and 1 and 9
    if (start_with_zero) {
      Seek(pos);
      return 0;
    } else {
      Seek(pos);
      return ch;
    }
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
    } else {
      // 0 pattern
      Advance();
    }
    if (core::character::IsDecimalDigit(c_)) {
      *e = UNEXPECTED_CHARACTER;
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
          ranges_.Add('-', false);
          break;
        } else {
          uint16_t ranged2 = 0;
          const uint16_t last = ParseClassAtom(&ranged2, CHECK);
          if (ranged1 || ranged2) {
            ranges_.AddOrEscaped(ranged1, start);
            ranges_.Add('-', false);
            ranges_.AddOrEscaped(ranged2, last);
          } else {
            if (!RangeBuilder::IsValidRange(start, last)) {
              RAISE(INVALID_RANGE);
            }
            ranges_.AddRange(start, last, true);
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
        case 'S':
        case 'n': {
          *ranged = c_;
          Advance();
          return 0;
        }
        case 'f': {
          Advance();
          return '\f';
        }
        case 'b': {
          Advance();
          return '\b';
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
            return 'c';
          }
          Advance();
          return '\\';
        }
        case 'x': {
          Advance();
          const int res = ParseHexEscape(2);
          return (res == -1) ? 'x' : res;
        }
        case 'u': {
          Advance();
          const int res = ParseHexEscape(4);
          return (res == -1) ? 'u' : res;
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
          if (c_ < 0) {
            *e = UNEXPECTED_CHARACTER;
            return 0;
          } else if (core::character::IsDecimalDigit(c_)) {
            const double numeric = ParseNumericClassEscape();
            const uint16_t uc = static_cast<uint16_t>(numeric);
            if (uc != numeric) {
              *e = NUMBER_TOO_BIG;
              return 0;
            }
            return uc;
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

  Expression* ParseQuantifier(Expression* target, bool* ok, int* e) {
    // this is extended ES5 RegExp
    // see http://wiki.ecmascript.org/doku.php?id=harmony:regexp_match_web_reality

    // when Quantifier parse is failed, seek to start position and return
    // original target Expression.
    const std::size_t pos = pos_;

    int32_t min = 0;
    int32_t max = 0;
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
        if (!core::character::IsDecimalDigit(c_)) {
          // recovery pattern
          Seek(pos);
          return target;
        }
        const double numeric1 = ParseDecimalInteger(e);
        if (*e) {
          // recovery pattern
          *e = NONE;
          Seek(pos);
          return target;
        }
        if (numeric1 > kRegExpInfinity) {
          min = kRegExpInfinity;
        } else {
          min = static_cast<int32_t>(numeric1);
          if (min != numeric1) {
            // not recovery pattern
            Seek(pos);
            RAISE(NUMBER_TOO_BIG);
          }
        }
        if (c_ == ',') {
          Advance();
          if (c_ == '}') {
            max = kRegExpInfinity;
          } else {
            if (!core::character::IsDecimalDigit(c_)) {
              // recovery pattern
              Seek(pos);
              return target;
            }
            const double numeric2 = ParseDecimalInteger(e);
            if (*e) {
              // recovery pattern
              *e = NONE;
              Seek(pos);
              return target;
            }
            if (numeric2 > kRegExpInfinity) {
              max = kRegExpInfinity;
            } else {
              max = static_cast<int32_t>(numeric2);
              if (max != numeric2) {
                // not recovery pattern
                Seek(pos);
                RAISE(NUMBER_TOO_BIG);
              }
            }
          }
        } else if (c_ == '}') {
          max = min;
        }
        if (c_ != '}') {
          // recovery pattern
          Seek(pos);
          return target;
        }
        Advance();
        break;
      }
      default: {
        // recovery pattern
        Seek(pos);
        return target;
      }
    }
    if (max < min) {
      // not recovery pattern
      Seek(pos);
      RAISE(INVALID_QUANTIFIER);
    }
    // postfix
    bool greedy = true;
    if (c_ == '?') {
      Advance();
      if (max != min) {
        greedy = false;
      }
    }
    *ok = true;
    if (min == max && min == 1) {
      assert(max == 1);
      return target;
    }
    return new(factory_)Quantifiered(target, min, max, greedy);
  }

  inline void Advance() {
    if (pos_ >= end_) {
      c_ = EOS;
      if (pos_ == end_) {
        ++pos_;
      }
    } else {
      c_ = source_[pos_++];
    }
  }

  void Seek(std::size_t pos) {
    assert(0 < pos);
    pos_ = pos - 1;
    Advance();
  }

  void PushBack() {
    if (pos_ < 2) {
      c_ = EOS;
    } else {
      c_ = source_[pos_ - 2];
      --pos_;
    }
  }

  int flags_;
  core::Space* factory_;
  RangeBuilder ranges_;
  const core::UStringPiece source_;
  std::vector<char> buffer8_;
  std::size_t pos_;
  const std::size_t end_;
  int c_;
  uint32_t captures_;
};

#undef EXPECT
#undef UNEXPECT
#undef RAISE
#undef CHECK
} }  // namespace iv::aero
#endif  // IV_AERO_PARSER_H_
