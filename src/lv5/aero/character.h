#ifndef IV_LV5_AERO_CHARACTER_H_
#define IV_LV5_AERO_CHARACTER_H_
#include "character.h"
namespace iv {
namespace lv5 {
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
      ch != '{' &&
      ch != '}' &&
      ch != '|';
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

} } } }  // namespace iv::lv5::aero::character
#endif  // IV_LV5_AERO_CHARACTER_H_
