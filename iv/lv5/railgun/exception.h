#ifndef IV_LV5_RAILGUN_EXCEPTION_H_
#define IV_LV5_RAILGUN_EXCEPTION_H_
#include <iv/detail/cstdint.h>
#include <iv/detail/tuple.h>
#include <iv/lv5/gc_template.h>
namespace iv {
namespace lv5 {
namespace railgun {

struct Handler {
  enum Type {
    CATCH,
    FINALLY,
    ITER
  };
};

typedef std::tuple<uint8_t, uint16_t,
                   uint16_t, uint16_t, uint16_t> ExceptionHandler;
typedef GCVector<ExceptionHandler>::type ExceptionTable;


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_EXCEPTION_H_
