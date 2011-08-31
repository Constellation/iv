#ifndef IV_LV5_GLOBAL_DATA_H_
#define IV_LV5_GLOBAL_DATA_H_
#include <ctime>
#include "detail/array.h"
#include "dtoa.h"
#include "conversions.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "ustring.h"
#include "random.h"
#include "lv5/class.h"
#include "lv5/gc_template.h"
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
  typedef core::UniformRandomGenerator<core::Xor128> RandomGenerator;

  GlobalData(Context* ctx)
    : random_generator_(0, 1, std::time(NULL)),
      regs_(),
      table_(),
      classes_(),
      string_cache_(),
      empty_(new JSString()),
      global_obj_(ctx) {
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
    return random_generator_.get();
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

 private:
  RandomGenerator random_generator_;
  trace::Vector<JSRegExpImpl*>::type regs_;
  SymbolTable table_;
  std::array<ClassSlot, Class::NUM_OF_CLASS> classes_;
  std::array<JSString*, 0xFF + 1> string_cache_;
  JSString* empty_;
  JSGlobal global_obj_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GLOBAL_DATA_H_
