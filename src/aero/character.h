#ifndef IV_AERO_CHARACTER_H_
#define IV_AERO_CHARACTER_H_
#include "character.h"
namespace iv {
namespace aero {
namespace character {

inline bool IsPatternCharacter(int ch) {
  return
      ch != '^' &&
      ch != '$' &&
      ch != '\\' &&
      ch != '.' &&
      ch != '*' &&
      ch != '+' &&
      ch != '?' &&
      ch != '(' &&
      ch != ')' &&
      ch != '[' &&
      ch != ']' &&
      ch != '|';
}

inline bool IsBrace(int ch) {
  return ch == '{' || ch == '}';
}

inline bool IsPatternCharacterNoBrace(int ch) {
  return IsPatternCharacter(ch) && !IsBrace(ch);
}

inline bool IsQuantifierPrefixStart(int ch) {
  return
      ch == '*' ||
      ch == '+' ||
      ch == '?' ||
      ch == '{';
}

inline bool IsWord(uint16_t ch) {
  return core::character::IsASCIIAlphanumeric(ch) || ch == '_';
}

} } }  // namespace iv::aero::character
#endif  // IV_AERO_CHARACTER_H_
