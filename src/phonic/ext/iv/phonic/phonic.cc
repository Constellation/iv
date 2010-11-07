#include <ruby.h>
#include "rparser.h"

#define RBFUNC(func) (reinterpret_cast<VALUE(*)(...)>(func))

extern "C" {

void Init_phonic() {
  iv::phonic::Encoding::Init();
  // IV
  VALUE mIV = rb_define_module("IV");

  // IV::Phonic
  VALUE mIVPhonic = rb_define_module_under(mIV, "Phonic");
  rb_define_module_function(mIVPhonic, "parse",
                            RBFUNC(&iv::phonic::RParser::Parse), 1);
  iv::phonic::RParser::Init(mIVPhonic);
}

}

#undef RBFUNC
