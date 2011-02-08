#ifndef _IV_PHONIC_PARSER_H_
#define _IV_PHONIC_PARSER_H_
#include <iv/parser.h>
#include "factory.h"
#include "source.h"
namespace iv {
namespace phonic {

typedef core::Parser<AstFactory, UTF16Source, true, false> Parser;

} }  // namespace iv::phonic
#endif  // _IV_PHONIC_PARSER_H_
