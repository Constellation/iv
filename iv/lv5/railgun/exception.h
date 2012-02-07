#ifndef IV_LV5_RAILGUN_EXCEPTION_H_
#define IV_LV5_RAILGUN_EXCEPTION_H_
#include <iv/detail/cstdint.h>
#include <iv/detail/tuple.h>
#include <iv/lv5/gc_template.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Handler {
 public:
  enum Type {
    CATCH,
    FINALLY,
    ITER
  };

  Handler(Type type,
          uint32_t begin,
          uint32_t end, 
          int32_t jmp,
          int32_t ret,
          int32_t flag,
          uint32_t dynamic_env_level)
    : type_(type),
      begin_(begin),
      end_(end),
      jmp_(jmp),
      ret_(ret),
      flag_(flag),
      dynamic_env_level_(dynamic_env_level) { }

  Type type() const { return type_; }

  uint32_t begin() const { return begin_; }

  uint32_t end() const { return end_; }

  int32_t jmp() const { return jmp_; }

  int32_t ret() const { return ret_; }

  int32_t flag() const { return flag_; }

  uint32_t dynamic_env_level() const { return dynamic_env_level_; }

 private:
  Type type_;
  uint32_t begin_;
  uint32_t end_;
  int32_t jmp_;
  int32_t ret_;
  int32_t flag_;
  uint32_t dynamic_env_level_;
};

typedef GCVector<Handler>::type ExceptionTable;


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_EXCEPTION_H_
