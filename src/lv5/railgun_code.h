#ifndef _IV_LV5_RAILGUN_CODE_H_
#define _IV_LV5_RAILGUN_CODE_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include <tr1/tuple>
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/railgun_fwd.h"
#include "lv5/railgun_op.h"
#include "lv5/gc_template.h"
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

class Code : public gc {
 public:
  friend class Compiler;
  typedef GCVector<Symbol>::type Names;
  typedef GCVector<uint8_t>::type Data;
  typedef GCVector<Code*>::type Codes;
  typedef std::tr1::tuple<uint8_t, uint16_t, uint16_t, uint16_t, uint16_t>
          ExceptionHandler;
  typedef GCVector<ExceptionHandler>::type ExceptionTable;

  explicit Code(bool strict)
    : strict_(strict),
      data_(),
      codes_(),
      names_(),
      varnames_(),
      constants_() {
  }

  const uint8_t* data() const {
    return data_.data();
  }

  const Codes& codes() const {
    return codes_;
  }

  const Data& Main() const {
    return data_;
  }

  const JSVals& constants() const {
    return constants_;
  }

  const Names& names() const {
    return names_;
  }

  const Names& varnames() const {
    return varnames_;
  }

  const ExceptionTable& exception_table() const {
    return exception_table_;
  }

  template<Handler::Type type>
  void RegisterHandler(uint16_t begin, uint16_t end,
                       uint16_t stack_base_level,
                       uint16_t dynamic_scope_depth) {
    exception_table_.push_back(
        std::tr1::make_tuple(type, begin, end, stack_base_level, dynamic_scope_depth));
  }

  bool strict() const {
    return strict_;
  }

 private:
  bool strict_;
  Data data_;
  Codes codes_;
  Names names_;
  Names varnames_;
  JSVals constants_;
  ExceptionTable exception_table_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CODE_H_
