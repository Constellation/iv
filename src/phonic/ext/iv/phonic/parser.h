#ifndef IV_PHONIC_PARSER_H_
#define IV_PHONIC_PARSER_H_
#include <iv/parser.h>
#include "factory.h"
#include "source.h"
namespace iv {
namespace phonic {

typedef core::Parser<AstFactory, UTF16Source, true, false> Parser;

} }  // namespace iv::phonic
#endif  // IV_PHONIC_PARSER_H_
