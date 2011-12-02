#ifndef IV_SYMBOL_H_
#define IV_SYMBOL_H_
#include <cstddef>
#include <iv/symbol_fwd.h>
#include <iv/default_symbol_provider.h>
namespace iv {
namespace core {

typedef const core::UString* StringSymbol;

namespace symbol {

static const Symbol kDummySymbol =
    detail::MakeSymbol(static_cast<core::UString*>(NULL));

} } }  // namespace iv::core::symbol
#endif  // IV_SYMBOL_H_
