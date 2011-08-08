#ifndef _IV_LV5_GLOBAL_DATA_H_
#define _IV_LV5_GLOBAL_DATA_H_
#include <boost/random.hpp>
#include "detail/array.h"
#include "dtoa.h"
#include "conversions.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "ustring.h"
#include "lv5/class.h"
#include "lv5/gc_template.h"
#include "lv5/xorshift.h"
#include "lv5/jsstring.h"
#include "lv5/jsfunction.h"
#include "lv5/jsglobal.h"
#include "lv5/symboltable.h"
namespace iv {
namespace lv5 {

class JSRegExpImpl;
class Context;

// GlobalData has symboltable, global object
class GlobalData {
 public:
  friend class Context;
  typedef Xor128 random_engine_type;
  typedef boost::uniform_real<double> random_distribution_type;
  typedef boost::variate_generator<
      random_engine_type, random_distribution_type> random_generator;

  GlobalData(Context* ctx)
    : random_engine_(random_engine_type(),
                     random_distribution_type(0, 1)),
      regs_(),
      table_(),
      classes_(),
      string_cache_(),
      empty_(new JSString()),
      global_obj_(ctx) {
    // discard random
    for (std::size_t i = 0; i < 20; ++i) {
      Random();
    }
  }

  Symbol Intern(const core::StringPiece& str) {
    return table_.Lookup(str);
  }

  Symbol Intern(const core::UStringPiece& str) {
    return table_.Lookup(str);
  }

  Symbol InternUInt32(uint32_t index) {
    return symbol::MakeSymbolFromIndex(index);
  }

  Symbol InternDouble(double number) {
    if (number == static_cast<uint32_t>(number)) {
      return InternUInt32(static_cast<uint32_t>(number));
    } else {
      std::array<char, 80> buffer;
      const char* const str = core::DoubleToCString(number,
                                                    buffer.data(),
                                                    buffer.size());
      return table_.Lookup(core::StringPiece(str));
    }
  }

  double Random() {
    return random_engine_();
  }

  const JSGlobal* global_obj() const {
    return &global_obj_;
  }

  JSGlobal* global_obj() {
    return &global_obj_;
  }

  void RegisterLiteralRegExp(JSRegExpImpl* reg) {
    regs_.push_back(reg); }

  template<Class::JSClassType CLS>
  void RegisterClass(const ClassSlot& slot) {
    classes_[CLS] = slot;
  }

  const ClassSlot& GetClassSlot(Class::JSClassType cls) const {
    return classes_[cls];
  }

  JSString* GetEmptyString() const {
    return empty_;
  }

  JSString* GetSingleString(uint16_t ch) {
    if (ch <= 0xFF) {
      // caching value
      if (string_cache_[ch]) {
        return string_cache_[ch];
      }
      return (string_cache_[ch] = new JSString(ch));
    }
    return NULL;
  }

  Map* GetGlobalMap() {
    return &map_;
  }

 private:
  random_generator random_engine_;
  trace::Vector<JSRegExpImpl*>::type regs_;
  SymbolTable table_;
  std::array<ClassSlot, Class::NUM_OF_CLASS> classes_;
  std::array<JSString*, 0xFF + 1> string_cache_;
  JSString* empty_;
  Map map_;
  JSGlobal global_obj_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_GLOBAL_DATA_H_
