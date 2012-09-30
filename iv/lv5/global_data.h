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
#include <iv/symbol_table.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/gc_hook.h>
#include <iv/lv5/jsstring_fwd.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/attributes.h>
#include <iv/lv5/binary_blocks.h>
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
      symbol_table_(),
      classes_(),
      string_cache_(),
      global_obj_(JSGlobal::New(ctx)),
      string_empty_(new JSString()),
      string_null_(),
      string_true_(),
      string_false_(),
      string_undefined_(),
      string_function_(),
      string_object_(),
      string_number_(),
      string_string_(),
      string_boolean_(),
      string_empty_regexp_(),
      empty_object_map_(Map::New(ctx)),
      function_map_(Map::New(ctx)),
      array_map_(Map::New(ctx)),
      string_map_(Map::New(ctx)),
      boolean_map_(Map::New(ctx)),
      number_map_(Map::New(ctx)),
      date_map_(Map::New(ctx)),
      regexp_map_(NULL),
      error_map_(Map::New(ctx)),
      map_map_(Map::New(ctx)),
      array_buffer_map_(NULL),
      data_view_map_(NULL),
      typed_array_map_(NULL),
      gc_hook_(this) {
    {
      Error::Dummy e;
      string_null_ = JSString::NewAsciiString(ctx, "null", &e);
      string_true_ = JSString::NewAsciiString(ctx, "true", &e);
      string_false_ = JSString::NewAsciiString(ctx, "false", &e);
      string_undefined_ = JSString::NewAsciiString(ctx, "undefined", &e);
      string_function_ = JSString::NewAsciiString(ctx, "function", &e);
      string_object_ = JSString::NewAsciiString(ctx, "object", &e);
      string_number_ = JSString::NewAsciiString(ctx, "number", &e);
      string_string_ = JSString::NewAsciiString(ctx, "string", &e);
      string_boolean_ = JSString::NewAsciiString(ctx, "boolean", &e);
      string_empty_regexp_ = JSString::NewAsciiString(ctx, "(?:)", &e);

      // RegExp Map
      // see also jsregexp.h, JSRegExp::FIELD
      {
        MapBuilder builder(ctx);
        builder.Add(symbol::source(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::global(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::ignoreCase(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::multiline(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::lastIndex(), ATTR::CreateData(ATTR::W));
        regexp_map_ = builder.Build();
      }

      // ArrayBuffer Map
      //   see also jsarray_buffer.h, JSArrayBuffer::FIELD
      {
        MapBuilder builder(ctx);
        builder.Add(symbol::byteLength(), ATTR::CreateData(ATTR::N));
        array_buffer_map_ = builder.Build();
      }

      // DataView Map
      //   see also jsdata_view.h, JSDataView::FIELD
      {
        MapBuilder builder(ctx);
        builder.Add(symbol::byteLength(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::buffer(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::byteOffset(), ATTR::CreateData(ATTR::N));
        data_view_map_ = builder.Build();
      }

      // TypedArray Map
      //   see also jstyped_array.h, TypedArrayImpl::FIELD
      {
        MapBuilder builder(ctx);
        builder.Add(symbol::length(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::byteLength(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::buffer(), ATTR::CreateData(ATTR::N));
        builder.Add(symbol::byteOffset(), ATTR::CreateData(ATTR::N));
        typed_array_map_ = builder.Build();
      }
    }
  }

  Symbol Intern(const core::StringPiece& str) {
    return symbol_table_.Lookup(str);
  }

  Symbol Intern(const core::UStringPiece& str) {
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
      return symbol_table_.Lookup(core::StringPiece(str));
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
          core::StringPiece(buffer.data(), last - buffer.data()));
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

  JSString* string_empty() const { return string_empty_; }

  JSString* string_null() const { return string_null_; }

  JSString* string_true() const { return string_true_; }

  JSString* string_false() const { return string_false_; }

  JSString* string_undefined() const { return string_undefined_; }

  JSString* string_function() const { return string_function_; }

  JSString* string_object() const { return string_object_; }

  JSString* string_number() const { return string_number_; }

  JSString* string_string() const { return string_string_; }

  JSString* string_boolean() const { return string_boolean_; }

  JSString* string_empty_regexp() const { return string_empty_regexp_; }

  JSString* GetSingleString(uint16_t ch) {
    if (ch < 0x80) {
      // caching value
      if (string_cache_[ch]) {
        return string_cache_[ch];
      }
      return (string_cache_[ch] = new JSString(ch));
    }
    return NULL;
  }

  Map* GetEmptyObjectMap() const { return empty_object_map_; }

  Map* GetFunctionMap() const { return function_map_; }

  Map* GetArrayMap() const { return array_map_; }

  Map* GetStringMap() const { return string_map_; }

  Map* GetBooleanMap() const { return boolean_map_; }

  Map* GetNumberMap() const { return number_map_; }

  Map* GetDateMap() const { return date_map_; }

  Map* GetRegExpMap() const { return regexp_map_; }

  Map* GetErrorMap() const { return error_map_; }

  Map* GetMapMap() const { return map_map_; }

  Map* GetArrayBufferMap() const { return array_buffer_map_; }

  Map* GetDataViewMap() const { return data_view_map_; }

  Map* GetTypedArrayMap() const { return typed_array_map_; }

  void OnGarbageCollect() { }

  void RegExpClear() { regs_.clear(); }

  core::SymbolTable* symbol_table() { return &symbol_table_; }

  void set_map_prototype(JSObject* proto) {
    map_prototype_ = proto;
  }

  JSObject* map_prototype() const { return map_prototype_; }

  void set_array_buffer_prototype(JSObject* proto) {
    array_buffer_prototype_ = proto;
  }

  JSObject* array_buffer_prototype() const { return array_buffer_prototype_; }

  void set_data_view_prototype(JSObject* proto) {
    data_view_prototype_ = proto;
  }

  JSObject* data_view_prototype() const { return data_view_prototype_; }

  void set_typed_array_prototype(TypedCode::Code code, JSObject* proto) {
    typed_array_prototypes_[code] = proto;
  }

  JSObject* typed_array_prototype(TypedCode::Code code) const { return typed_array_prototypes_[code]; }


 private:
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
  JSString* string_boolean_;
  JSString* string_empty_regexp_;

  // prototypes
  JSObject* object_prototype_;
  JSObject* function_prototype_;
  JSObject* array_prototype_;
  JSObject* string_prototype_;
  JSObject* boolean_prototype_;
  JSObject* number_prototype_;
  JSObject* date_prototype_;
  JSObject* regexp_prototype_;
  JSObject* error_prototype_;
  JSObject* map_prototype_;
  JSObject* array_buffer_prototype_;
  JSObject* data_view_prototype_;
  std::array<JSObject*, TypedCode::kNumOfCodes> typed_array_prototypes_;

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
  Map* map_map_;
  Map* array_buffer_map_;
  Map* data_view_map_;
  Map* typed_array_map_;

  GCHook<GlobalData> gc_hook_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GLOBAL_DATA_H_
