#ifndef _IV_PHONIC_PARSER_H_
#define _IV_PHONIC_PARSER_H_
#include <ruby.h>
#include <iv/parser.h>
#include "factory.h"
namespace iv {
namespace phonic {

typedef core::Parser<AstFactory> Parser;

} }  // namespace iv::phonic
#endif  // _IV_PHONIC_PARSER_H_
