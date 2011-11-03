#ifndef IV_LV5_GLOBAL_DATA_H_
#define IV_LV5_GLOBAL_DATA_H_
#include <ctime>
#include <iv/detail/array.h>
#include <iv/dtoa.h>
#include <iv/conversions.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/ustring.h>
#include <iv/xorshift.h>
#include <iv/random_generator.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/gc_hook.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/symboltable.h>
namespace iv {
namespace lv5 {

class JSRegExpImpl;
class Context;

// GlobalData has symboltable, global object
class GlobalData {
 public:
  friend class Context;
  typedef core::UniformRandomGenerator<core::Xor128> RandomGenerator;

  explicit GlobalData(Context* ctx)
    : random_generator_(0, 1, static_cast<int>(std::time(NULL))),
      regs_(),
      table_(),
      classes_(),
      string_cache_(),
      empty_(new JSString()),
      global_obj_(JSGlobal::New(ctx)),
      empty_object_map_(Map::New(ctx)),
      function_map_(Map::New(ctx)),
      array_map_(Map::New(ctx)),
      string_map_(Map::New(ctx)),
      boolean_map_(Map::New(ctx)),
      number_map_(Map::New(ctx)),
      date_map_(Map::New(ctx)),
      regexp_map_(Map::New(ctx)),
      error_map_(Map::New(ctx)),
      gc_hook_(this) {
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

  JSGlobal* global_obj() const {
    return global_obj_;
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

  Map* GetEmptyObjectMap() const {
    return empty_object_map_;
  }

  Map* GetFunctionMap() const {
    return function_map_;
  }

  Map* GetArrayMap() const {
    return array_map_;
  }

  Map* GetStringMap() const {
    return string_map_;
  }

  Map* GetBooleanMap() const {
    return boolean_map_;
  }

  Map* GetNumberMap() const {
    return number_map_;
  }

  Map* GetDateMap() const {
    return date_map_;
  }

  Map* GetRegExpMap() const {
    return regexp_map_;
  }

  Map* GetErrorMap() const {
    return error_map_;
  }

  void OnGarbageCollect() {
  }

  void RegExpClear() { regs_.clear(); }

 private:
  RandomGenerator random_generator_;
  trace::Vector<JSRegExpImpl*>::type regs_;
  SymbolTable table_;
  std::array<ClassSlot, Class::NUM_OF_CLASS> classes_;
  std::array<JSString*, 0xFF + 1> string_cache_;
  JSString* empty_;
  JSGlobal* global_obj_;

  // builtin maps
  Map* empty_object_map_;
  Map* function_map_;
  Map* array_map_;
  Map* string_map_;
  Map* boolean_map_;
  Map* number_map_;
  Map* date_map_;
  Map* regexp_map_;
  Map* error_map_;

  GCHook<GlobalData> gc_hook_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GLOBAL_DATA_H_
