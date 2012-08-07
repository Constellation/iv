// PolyIC compiler
#ifndef IV_BREAKER_POLY_IC_H_
#define IV_BREAKER_POLY_IC_H_
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/ic.h>
namespace iv {
namespace lv5 {
namespace breaker {

class PolyIC : public IC {
};

class LoadPropertyIC : public PolyIC {
};

class StorePropertyIC : public PolyIC {
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_POLY_IC_H_
