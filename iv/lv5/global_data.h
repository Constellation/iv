#ifndef IV_LV5_GLOBAL_DATA_H_
#define IV_LV5_GLOBAL_DATA_H_
#include <string>
#include <iv/detail/array.h>
#include <iv/dtoa.h>
#include <iv/conversions.h>
#include <iv/string_view.h>
#include <iv/xorshift.h>
#include <iv/random_generator.h>
#include <iv/symbol_table.h>
#include <iv/lv5/class.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/gc_hook.h>
#include <iv/lv5/binary_blocks.h>
#include <iv/lv5/global_symbols.h>
namespace iv {
namespace lv5 {

class JSGlobal;
class JSString;
class JSRegExpImpl;
class Context;
class Map;

// GlobalData has symboltable, global object
class GlobalData : public GlobalSymbols {
 public:
  friend class Context;
  typedef core::UniformRandomGenerator<core::Xor128> RandomGenerator;
  typedef std::array<JSObject*, TypedCode::kNumOfCodes> TypedArrayPrototypes;
  typedef std::array<Map*, TypedCode::kNumOfCodes> TypedArrayMaps;

  explicit GlobalData(Context* ctx);

  void InitNormalObjectMaps(Context* ctx);

  Symbol Intern(const core::string_view& str) {
    return symbol_table_.Lookup(str);
  }

  Symbol Intern(const core::u16string_view& str) {
    return symbol_table_.Lookup(str);
  }

  Symbol InternUInt32(uint32_t index) {
    return symbol::MakeSymbolFromIndex(index);
  }

  Symbol InternDouble(double number) {
    const uint32_t converted = static_cast<uint32_t>(number);
    if (number == converted) {
      return InternUInt32(converted);
    } else {
      std::array<char, 80> buffer;
      const char* const str = core::DoubleToCString(number,
                                                    buffer.data(),
                                                    buffer.size());
      return symbol_table_.Lookup(core::string_view(str));
    }
  }

  Symbol Intern64(uint64_t val) {
    const uint32_t converted = static_cast<uint32_t>(val);
    if (val == converted) {
      return InternUInt32(converted);
    } else {
      std::array<char, 30> buffer;
      const char* last = core::UInt64ToString(val, buffer.data());
      return symbol_table_.Lookup(
          core::string_view(buffer.data(), last - buffer.data()));
    }
  }

  Context* ctx() const { return ctx_; }

  double Random() { return random_generator_.get(); }

  JSGlobal* global_obj() const { return global_obj_; }

  void RegisterLiteralRegExp(JSRegExpImpl* reg) {
    regs_.push_back(reg); }

  template<Class::JSClassType CLS>
  void RegisterClass(const ClassSlot& slot) {
    classes_[CLS] = slot;
  }

  const ClassSlot& GetClassSlot(Class::JSClassType cls) const {
    return classes_[cls];
  }

  JSString* string_empty() const { return string_empty_; }
  JSString* string_null() const { return string_null_; }
  JSString* string_true() const { return string_true_; }
  JSString* string_false() const { return string_false_; }
  JSString* string_undefined() const { return string_undefined_; }
  JSString* string_function() const { return string_function_; }
  JSString* string_object() const { return string_object_; }
  JSString* string_number() const { return string_number_; }
  JSString* string_string() const { return string_string_; }
  JSString* string_symbol() const { return string_symbol_; }
  JSString* string_boolean() const { return string_boolean_; }
  JSString* string_empty_regexp() const { return string_empty_regexp_; }

  // If string is not cached, return nullptr
  JSString* GetSingleString(char16_t ch);

  Map* primitive_string_map() const { return primitive_string_map_; }
  Map* primitive_symbol_map() const { return primitive_symbol_map_; }
  Map* empty_object_map() const { return empty_object_map_; }
  Map* function_map() const { return function_map_; }
  Map* array_map() const { return array_map_; }
  Map* array_iterator_map() const { return array_iterator_map_; }
  Map* string_map() const { return string_map_; }
  Map* string_iterator_map() const { return string_iterator_map_; }
  Map* symbol_map() const { return symbol_map_; }
  Map* boolean_map() const { return boolean_map_; }
  Map* number_map() const { return number_map_; }
  Map* date_map() const { return date_map_; }
  Map* regexp_map() const { return regexp_map_; }
  Map* error_map() const { return error_map_; }
  Map* eval_error_map() const { return eval_error_map_; }
  Map* range_error_map() const { return range_error_map_; }
  Map* reference_error_map() const { return reference_error_map_; }
  Map* syntax_error_map() const { return syntax_error_map_; }
  Map* type_error_map() const { return type_error_map_; }
  Map* uri_error_map() const { return uri_error_map_; }
  Map* map_map() const { return map_map_; }
  Map* map_iterator_map() const { return map_iterator_map_; }
  Map* weak_map_map() const { return weak_map_map_; }
  Map* set_map() const { return set_map_; }
  Map* set_iterator_map() const { return set_iterator_map_; }
  Map* array_buffer_map() const { return array_buffer_map_; }
  Map* data_view_map() const { return data_view_map_; }
  Map* iterator_map() const { return iterator_map_; }
  Map* iterator_result_map() const { return iterator_result_map_; }
  Map* typed_array_map(TypedCode::Code code) const { return typed_array_maps_[code]; }
  Map* normal_arguments_map() const { return normal_arguments_map_; }
  Map* strict_arguments_map() const { return strict_arguments_map_; }
  Map* number_format_map() const { return number_format_map_; }
  Map* date_time_format_map() const { return date_time_format_map_; }

  void InitArgumentsMap();

  void OnGarbageCollect() {
  }

  void RegExpClear() { regs_.clear(); }

  core::SymbolTable* symbol_table() { return &symbol_table_; }

  // prototypes getter
  JSObject* object_prototype() const { return object_prototype_; }
  JSObject* function_prototype() const { return function_prototype_; }
  JSObject* array_prototype() const { return array_prototype_; }
  JSObject* array_iterator_prototype() const { return array_iterator_prototype_; }
  JSObject* string_prototype() const { return string_prototype_; }
  JSObject* string_iterator_prototype() const { return string_iterator_prototype_; }
  JSObject* symbol_prototype() const { return symbol_prototype_; }
  JSObject* boolean_prototype() const { return boolean_prototype_; }
  JSObject* number_prototype() const { return number_prototype_; }
  JSObject* date_prototype() const { return date_prototype_; }
  JSObject* regexp_prototype() const { return regexp_prototype_; }
  JSObject* error_prototype() const { return error_prototype_; }
  JSObject* eval_error_prototype() const { return eval_error_prototype_; }
  JSObject* range_error_prototype() const { return range_error_prototype_; }
  JSObject* reference_error_prototype() const { return reference_error_prototype_; }
  JSObject* syntax_error_prototype() const { return syntax_error_prototype_; }
  JSObject* type_error_prototype() const { return type_error_prototype_; }
  JSObject* uri_error_prototype() const { return uri_error_prototype_; }
  JSObject* iterator_prototype() const { return iterator_prototype_; }
  JSObject* map_prototype() const { return map_prototype_; }
  JSObject* map_iterator_prototype() const { return map_iterator_prototype_; }
  JSObject* weak_map_prototype() const { return weak_map_prototype_; }
  JSObject* set_prototype() const { return set_prototype_; }
  JSObject* set_iterator_prototype() const { return set_iterator_prototype_; }
  JSObject* array_buffer_prototype() const { return array_buffer_prototype_; }
  JSObject* data_view_prototype() const { return data_view_prototype_; }
  JSObject* typed_array_prototype(TypedCode::Code code) const { return typed_array_prototypes_[code]; }
  JSObject* number_format_prototype() const { return number_format_prototype_; }
  JSObject* date_time_format_prototype() const { return date_time_format_prototype_; }

 private:
  // prototypes setter
  void set_object_prototype(JSObject* proto) { object_prototype_ = proto; }
  void set_function_prototype(JSObject* proto) { function_prototype_ = proto; }
  void set_array_prototype(JSObject* proto) { array_prototype_ = proto; }
  void set_array_iterator_prototype(JSObject* proto) { array_iterator_prototype_ = proto; }
  void set_string_prototype(JSObject* proto) { string_prototype_ = proto; }
  void set_string_iterator_prototype(JSObject* proto) { string_iterator_prototype_ = proto; }
  void set_symbol_prototype(JSObject* proto) { symbol_prototype_ = proto; }
  void set_boolean_prototype(JSObject* proto) { boolean_prototype_ = proto; }
  void set_number_prototype(JSObject* proto) { number_prototype_ = proto; }
  void set_date_prototype(JSObject* proto) { date_prototype_ = proto; }
  void set_regexp_prototype(JSObject* proto) { regexp_prototype_ = proto; }
  void set_error_prototype(JSObject* proto) { error_prototype_ = proto; }
  void set_eval_error_prototype(JSObject* proto) { eval_error_prototype_ = proto; }
  void set_range_error_prototype(JSObject* proto) { range_error_prototype_ = proto; }
  void set_reference_error_prototype(JSObject* proto) { reference_error_prototype_ = proto; }
  void set_syntax_error_prototype(JSObject* proto) { syntax_error_prototype_ = proto; }
  void set_type_error_prototype(JSObject* proto) { type_error_prototype_ = proto; }
  void set_uri_error_prototype(JSObject* proto) { uri_error_prototype_ = proto; }
  void set_map_prototype(JSObject* proto) { map_prototype_ = proto; }
  void set_map_iterator_prototype(JSObject* proto) { map_iterator_prototype_ = proto; }
  void set_weak_map_prototype(JSObject* proto) { weak_map_prototype_ = proto; }
  void set_set_prototype(JSObject* proto) { set_prototype_ = proto; }
  void set_set_iterator_prototype(JSObject* proto) { set_iterator_prototype_ = proto; }
  void set_array_buffer_prototype(JSObject* proto) { array_buffer_prototype_ = proto; }
  void set_data_view_prototype(JSObject* proto) { data_view_prototype_ = proto; }
  void set_typed_array_prototype(TypedCode::Code code, JSObject* proto) { typed_array_prototypes_[code] = proto; }
  void set_number_format_prototype(JSObject* proto) { number_format_prototype_ = proto; }
  void set_date_time_format_prototype(JSObject* proto) { date_time_format_prototype_ = proto; }


  Context* ctx_;

  RandomGenerator random_generator_;
  trace::Vector<JSRegExpImpl*>::type regs_;
  core::SymbolTable symbol_table_;
  std::array<ClassSlot, Class::NUM_OF_CLASS> classes_;
  std::array<JSString*, 0x80> string_cache_;
  JSGlobal* global_obj_;

  // cached strings
  JSString* string_empty_;
  JSString* string_null_;
  JSString* string_true_;
  JSString* string_false_;
  JSString* string_undefined_;
  JSString* string_function_;
  JSString* string_object_;
  JSString* string_number_;
  JSString* string_string_;
  JSString* string_symbol_;
  JSString* string_boolean_;
  JSString* string_empty_regexp_;

  // prototypes
  JSObject* object_prototype_;
  JSObject* function_prototype_;
  JSObject* array_prototype_;
  JSObject* array_iterator_prototype_;
  JSObject* string_prototype_;
  JSObject* string_iterator_prototype_;
  JSObject* symbol_prototype_;
  JSObject* boolean_prototype_;
  JSObject* number_prototype_;
  JSObject* date_prototype_;
  JSObject* regexp_prototype_;
  JSObject* error_prototype_;
  JSObject* eval_error_prototype_;
  JSObject* range_error_prototype_;
  JSObject* reference_error_prototype_;
  JSObject* syntax_error_prototype_;
  JSObject* type_error_prototype_;
  JSObject* uri_error_prototype_;
  JSObject* iterator_prototype_;
  JSObject* map_prototype_;
  JSObject* map_iterator_prototype_;
  JSObject* weak_map_prototype_;
  JSObject* set_prototype_;
  JSObject* set_iterator_prototype_;
  JSObject* array_buffer_prototype_;
  JSObject* data_view_prototype_;
  TypedArrayPrototypes typed_array_prototypes_;
  JSObject* number_format_prototype_;
  JSObject* date_time_format_prototype_;

  // builtin maps
  Map* primitive_string_map_;
  Map* primitive_symbol_map_;
  Map* empty_object_map_;
  Map* function_map_;
  Map* array_map_;
  Map* array_iterator_map_;
  Map* string_map_;
  Map* string_iterator_map_;
  Map* symbol_map_;
  Map* boolean_map_;
  Map* number_map_;
  Map* date_map_;
  Map* regexp_map_;
  Map* error_map_;
  Map* eval_error_map_;
  Map* range_error_map_;
  Map* reference_error_map_;
  Map* syntax_error_map_;
  Map* type_error_map_;
  Map* uri_error_map_;
  Map* map_map_;
  Map* map_iterator_map_;
  Map* weak_map_map_;
  Map* set_map_;
  Map* set_iterator_map_;
  Map* array_buffer_map_;
  Map* data_view_map_;
  Map* iterator_map_;
  Map* iterator_result_map_;
  TypedArrayMaps typed_array_maps_;
  Map* normal_arguments_map_;
  Map* strict_arguments_map_;
  Map* number_format_map_;
  Map* date_time_format_map_;

  GCHook<GlobalData> gc_hook_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GLOBAL_DATA_H_
