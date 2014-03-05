#ifndef IV_AERO_SIMPLE_CASE_H_
#define IV_AERO_SIMPLE_CASE_H_
#include <memory>
#include <string>
#include <iv/platform.h>
#include <iv/character.h>
#include <iv/aero/parser.h>
#include <iv/aero/flags.h>
namespace iv {
namespace aero {

class SimpleCase {
 public:
  SimpleCase(const StringAtom* str)
      : u8_()
      , u16_(str->string().begin(), str->string().end()) {
    for (char16_t ch : u16_) {
      if (!core::character::IsASCII(ch)) {
        return;
      }
    }
    u8_.assign(u16_.begin(), u16_.end());
  }

  static std::shared_ptr<SimpleCase> New(const ParsedData& data, bool ignore) {
    // Currently disabled.
    return nullptr;

    if (ignore) {
      return nullptr;
    }

    if (const Disjunction* dis = data.pattern()->AsDisjunction()) {
      if (dis->alternatives().size() != 1) {
        return nullptr;
      }
      const Alternative* alt = dis->alternatives().front();
      if (alt->terms().size() != 1) {
        return nullptr;
      }
      const Expression* exp = alt->terms().front();
      if (const StringAtom* str = exp->AsStringAtom()) {
        return std::shared_ptr<SimpleCase>(new SimpleCase(str));
      }
    }
    return nullptr;
  }

  int Execute(
      const core::UStringPiece& subject, int* captures, int offset) const {
    const std::size_t pos = subject.find(u16_, offset);
    if (core::UStringPiece::npos == pos) {
      return AERO_FAILURE;
    }
    captures[0] = pos;
    captures[1] = pos + u16_.size();
    return AERO_SUCCESS;
  }

  int Execute(
      const core::StringPiece& subject, int* captures, int offset) const {
    if (u8_.empty()) {
      return AERO_FAILURE;
    }
    const std::size_t pos = subject.find(u8_, offset);
    if (core::StringPiece::npos == pos) {
      return AERO_FAILURE;
    }
    captures[0] = pos;
    captures[1] = pos + u8_.size();
    return AERO_SUCCESS;
  }

  const std::string& u8() const { return u8_; }
  const std::u16string& u16() const { return u16_; }
 private:
  std::string u8_;
  std::u16string u16_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_SIMPLE_CASE_H_
