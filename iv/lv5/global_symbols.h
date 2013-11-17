#ifndef IV_LV5_GLOBAL_SYMBOLS_H_
#define IV_LV5_GLOBAL_SYMBOLS_H_
#include <iv/concat.h>
#include <iv/default_symbol_provider.h>
namespace iv {
namespace lv5 {

class JSSymbol;
class Context;

class GlobalSymbols {
 public:
#define V(name)\
  inline JSSymbol* IV_CONCAT(builtin_symbol_, name)() const {\
    return IV_CONCAT(IV_CONCAT(builtin_symbol_, name), _);\
  }\
  JSSymbol* IV_CONCAT(IV_CONCAT(builtin_symbol_, name), _);
  IV_BUILTIN_SYMBOLS(V)
#undef V

 protected:
  void InitGlobalSymbols(Context* ctx);
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GLOBAL_DATA_H_
