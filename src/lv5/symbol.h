#ifndef IV_LV5_SYMBOL_H_
#define IV_LV5_SYMBOL_H_
#include <cstddef>
#include "lv5/symbol_fwd.h"
#include "lv5/default_symbol_provider.h"
namespace iv {
namespace lv5 {

typedef const core::UString* StringSymbol;

namespace symbol {

static const Symbol kDummySymbol =
    detail::MakeSymbol(static_cast<core::UString*>(NULL));

} } }  // namespace iv::lv5::symbol
#endif  // IV_LV5_SYMBOL_H_
