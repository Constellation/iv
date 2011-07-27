// analyze condition value
//
// condition value is
//   TRUE / FALSE / INDETERMINATE
//
#ifndef _IV_LV5_RAILGUN_CONDITION_H_
#define _IV_LV5_RAILGUN_CONDITION_H_
#include "lv5/specialized_ast.h"
#include "lv5/railgun/fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Condition {
 public:
  enum Type {
    COND_FALSE = 0,
    COND_TRUE = 1,
    COND_INDETERMINATE = 2
  };

  static Type Analyze(const Expression* expr) {
    const Literal* literal = expr->AsLiteral();
    if (!literal) {
      return COND_INDETERMINATE;
    }

    if (literal->AsTrueLiteral()) {
      return COND_TRUE;
    }

    if (literal->AsFalseLiteral()) {
      return COND_FALSE;
    }

    if (const NumberLiteral* num = literal->AsNumberLiteral()) {
      const double val = num->value();
      if (val != 0 && !core::IsNaN(val)) {
        return COND_TRUE;
      } else {
        return COND_FALSE;
      }
    }

    if (const StringLiteral* str = literal->AsStringLiteral()) {
      if (str->value().empty()) {
        return COND_FALSE;
      } else {
        return COND_TRUE;
      }
    }

    if (literal->AsNullLiteral()) {
      return COND_FALSE;
    }

    return COND_INDETERMINATE;
  }
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CONDITION_H_
