#include <ctime>
#include <iv/detail/array.h>
#include <iv/dtoa.h>
#include <iv/conversions.h>
#include <iv/string_view.h>
#include <iv/ustring.h>
#include <iv/xorshift.h>
#include <iv/random_generator.h>
#include <iv/symbol_table.h>
#include <iv/lv5/map.h>
#include <iv/lv5/map_builder.h>
#include <iv/lv5/class.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/gc_hook.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsfunction_fwd.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/attributes.h>
#include <iv/lv5/binary_blocks.h>
#include <iv/lv5/global_symbols.h>
#include <iv/lv5/global_data.h>
namespace iv {
namespace lv5 {

GlobalData::GlobalData(Context* ctx)
  : ctx_(ctx),
    random_generator_(0, 1, static_cast<int>(std::time(nullptr))),
    regs_(),
    symbol_table_(),
    classes_(),
    string_cache_(),
    global_obj_(JSGlobal::New(ctx)),
    string_empty_(nullptr),
    string_null_(),
    string_true_(),
    string_false_(),
    string_undefined_(),
    string_function_(),
    string_object_(),
    string_number_(),
    string_string_(),
    string_symbol_(),
    string_boolean_(),
    string_empty_regexp_(),
    primitive_string_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    primitive_symbol_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    empty_object_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    function_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    array_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    array_iterator_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    string_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    string_iterator_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    symbol_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    boolean_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    number_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    date_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    regexp_map_(nullptr),
    error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    eval_error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    range_error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    reference_error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    syntax_error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    type_error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    uri_error_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    map_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    map_iterator_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    weak_map_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    set_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    set_iterator_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), true)),
    array_buffer_map_(nullptr),
    data_view_map_(nullptr),
    iterator_map_(nullptr),
    iterator_result_map_(nullptr),
    typed_array_maps_(),
    normal_arguments_map_(nullptr),
    strict_arguments_map_(nullptr),
    number_format_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    date_time_format_map_(Map::New(ctx, static_cast<JSObject*>(nullptr), false)),
    gc_hook_(this) {
  {
    Error::Dummy e;
    string_empty_ = new JSString(ctx);
    string_null_ = JSString::New(ctx, "null", &e);
    string_true_ = JSString::New(ctx, "true", &e);
    string_false_ = JSString::New(ctx, "false", &e);
    string_undefined_ = JSString::New(ctx, "undefined", &e);
    string_function_ = JSString::New(ctx, "function", &e);
    string_object_ = JSString::New(ctx, "object", &e);
    string_number_ = JSString::New(ctx, "number", &e);
    string_string_ = JSString::New(ctx, "string", &e);
    string_symbol_ = JSString::New(ctx, "symbol", &e);
    string_boolean_ = JSString::New(ctx, "boolean", &e);
    string_empty_regexp_ = JSString::New(ctx, "(?:)", &e);

    // RegExp Map
    // see also jsregexp.h, JSRegExp::FIELD
    {
      MapBuilder builder(ctx, nullptr);
      builder.Add(symbol::source(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::global(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::ignoreCase(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::multiline(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::lastIndex(), ATTR::CreateData(ATTR::W));
      regexp_map_ = builder.Build(false);
    }

    // ArrayBuffer Map
    //   see also jsarray_buffer.h, JSArrayBuffer::FIELD
    {
      MapBuilder builder(ctx, nullptr);
      builder.Add(symbol::byteLength(), ATTR::CreateData(ATTR::N));
      array_buffer_map_ = builder.Build(false);
    }

    // DataView Map
    //   see also jsdata_view.h, JSDataView::FIELD
    {
      MapBuilder builder(ctx, nullptr);
      builder.Add(symbol::byteLength(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::buffer(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::byteOffset(), ATTR::CreateData(ATTR::N));
      data_view_map_ = builder.Build(false);
    }

    // TypedArray Map
    //   see also jstyped_array.h, TypedArrayImpl::FIELD
    for (TypedArrayMaps::iterator it = typed_array_maps_.begin(),
         last = typed_array_maps_.end(); it != last; ++it) {
      MapBuilder builder(ctx, nullptr);
      builder.Add(symbol::length(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::byteLength(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::buffer(), ATTR::CreateData(ATTR::N));
      builder.Add(symbol::byteOffset(), ATTR::CreateData(ATTR::N));
      *it = builder.Build(false, true);
    }

    InitGlobalSymbols(ctx);
  }
}

void GlobalData::InitNormalObjectMaps(Context* ctx) {
  // Iterator Map
  uint32_t next_offset;
  iterator_map_ =
      empty_object_map()
        ->AddPropertyTransition(
            ctx,
            symbol::next(),
            ATTR::Object::Data(),
            &next_offset
        );

  // IteratorResult Map
  uint32_t value_offset, done_offset;
  iterator_result_map_ =
      empty_object_map()
        ->AddPropertyTransition(
            ctx,
            symbol::value(),
            ATTR::Object::Data(),
            &value_offset
        )
        ->AddPropertyTransition(
            ctx,
            symbol::done(),
            ATTR::Object::Data(),
            &done_offset
        );
  // assert(value_offset == JSIteratorResult::VALUE);
  // assert(done_offset  == JSIteratorResult::DONE);
}

// If string is not cached, return nullptr
JSString* GlobalData::GetSingleString(char16_t ch) {
  if (ch < 0x80) {
    // caching value
    const char ascii = ch;
    if (string_cache_[ch]) {
      return string_cache_[ch];
    }
    return (string_cache_[ch] =
            new JSSeqString(ctx_, true, &ascii, &ascii + 1));
  }
  return nullptr;
}

void GlobalData::InitArgumentsMap() {
  JSObject* proto = object_prototype();
  // normal arguments map
  {
    MapBuilder builder(ctx(), proto);
    builder.Add(symbol::length(), ATTR::CreateData(ATTR::W | ATTR::C));
    builder.Add(symbol::callee(), ATTR::CreateData(ATTR::W | ATTR::C));
    normal_arguments_map_ = builder.Build(false, true);
  }
  // strict arguments map
  {
    MapBuilder builder(ctx(), proto);
    builder.Add(symbol::length(), ATTR::CreateData(ATTR::W | ATTR::C));
    builder.Add(symbol::callee(), ATTR::CreateAccessor(ATTR::N));
    builder.Add(symbol::caller(), ATTR::CreateAccessor(ATTR::N));
    strict_arguments_map_ = builder.Build(false, true);
  }
}

} }  // namespace iv::lv5
