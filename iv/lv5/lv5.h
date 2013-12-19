#ifndef IV_LV5_LV5_H_
#define IV_LV5_LV5_H_
#include <gc/gc.h>
#include <iv/utils.h>
#include <iv/ast.h>
#include <iv/ast_serializer.h>
#include <iv/parser.h>
#include <iv/singleton.h>
#include <iv/file_source.h>
#include <iv/lv5/factory.h>
#include <iv/lv5/context.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/command.h>
#include <iv/lv5/fpu.h>
#include <iv/lv5/program.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/breaker.h>
#include <iv/lv5/radio/radio.h>
#include <iv/lv5/jsobject.h>
namespace iv {
namespace lv5 {

class Lv5Initializer : public core::Singleton<Lv5Initializer> {
 public:
  friend class core::Singleton<Lv5Initializer>;
  Lv5Initializer() {
    GC_INIT();
  }
};

inline void Init() {
  Lv5Initializer::Instance();
}

} }  // namespace iv::lv5
#endif  // IV_LV5_LV5_H_
