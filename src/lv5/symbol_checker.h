#ifndef _IV_LV5_SYMBOL_CHECKER_H_
#define _IV_LV5_SYMBOL_CHECKER_H_
#include "stringpiece.h"
#include "lv5/symbol.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

class SymbolChecker {
 public:
  typedef void (SymbolChecker::*bool_type)() const;

  SymbolChecker(Context* ctx, const core::StringPiece& str)
    : found_(false),
      sym_(context::Lookup(ctx, str, &found_)) {
  }

  SymbolChecker(Context* ctx, const core::UStringPiece& str)
    : found_(false),
      sym_(context::Lookup(ctx, str, &found_)) {
  }

  SymbolChecker(Context* ctx, uint32_t index)
    : found_(false),
      sym_(context::Lookup(ctx, index, &found_)) {
  }

  SymbolChecker(Context* ctx, double number)
    : found_(false),
      sym_(context::Lookup(ctx, number, &found_)) {
  }

  operator bool_type() const {
    return found_ ? &SymbolChecker::this_type_does_not_support_comparisons : 0;
  }

  Symbol symbol() const {
    return sym_;
  }

 private:
  void this_type_does_not_support_comparisons() const { }

  bool found_;
  Symbol sym_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_SYMBOL_CHECKER_H_
