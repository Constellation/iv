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
    ITERATOR,
    ENV
  };

  Handler(Type type,
          uint32_t begin,
          uint32_t end,
          int16_t jmp,
          int16_t ret,
          int16_t flag)
    : type_(type),
      jmp_(jmp),
      ret_(ret),
      flag_(flag),
      begin_(begin),
      end_(end) { }

  Type type() const { return type_; }

  uint32_t begin() const { return begin_; }

  uint32_t end() const { return end_; }

  void* program_counter_begin() const { return program_counter_begin_; }

  void* program_counter_end() const { return program_counter_end_; }

  void set_program_counter_begin(void* ptr) {
    program_counter_begin_ = ptr;
  }

  void set_program_counter_end(void* ptr) {
    program_counter_end_ = ptr;
  }

  int16_t jmp() const { return jmp_; }

  int16_t ret() const { return ret_; }

  int16_t flag() const { return flag_; }

 private:
  Type type_;
  int16_t jmp_;
  int16_t ret_;
  int16_t flag_;
  uint32_t begin_;
  uint32_t end_;
  void* program_counter_begin_;
  void* program_counter_end_;
};

typedef GCVector<Handler>::type ExceptionTable;


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_EXCEPTION_H_
