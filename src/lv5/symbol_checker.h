#ifndef _IV_LV5_SYMBOL_CHECKER_H_
#define _IV_LV5_SYMBOL_CHECKER_H_
#include "stringpiece.h"
#include "noncopyable.h"
#include "lv5/symbol.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

class SymbolChecker : private core::Noncopyable<SymbolChecker>::type {
 public:
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

  bool Found() const {
    return found_;
  }

  Symbol symbol() const {
    return sym_;
  }

 private:
  bool found_;
  Symbol sym_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_SYMBOL_CHECKER_H_
