#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsstring_iterator.h>
#include <iv/lv5/jsiterator_result.h>
#include <iv/lv5/runtime/object.h>
#include <iv/lv5/runtime/string_iterator.h>
namespace iv {
namespace lv5 {
namespace runtime {

// ES6
// section 21.1.5.2.1 %StringIteratorPrototype%.next()
JSVal StringIteratorNext(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%StringIterator%.next", args, e);
  Context* ctx = args.ctx();
  const JSVal O = args.this_binding();
  if (!O.IsObject()) {
    e->Report(Error::Type, "StringIterator next method only accepts object");
    return JSEmpty;
  }
  JSStringIterator* iterator = JSStringIterator::As(O.object());
  if (!iterator) {
    e->Report(Error::Type,
              "StringIterator next method only accepts StringIterator");
    return JSEmpty;
  }
  assert(iterator);

  JSString* s = iterator->Get<JSStringIterator::ITERATED>();
  if (!s) {
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  const uint32_t index = iterator->Get<JSStringIterator::INDEX>();
  const uint32_t len = s->size();

  if (index >= len) {
    iterator->Put<JSStringIterator::ITERATED>(nullptr);
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  const char16_t first = s->At(index);
  JSString* result = nullptr;
  if (first < 0xD800U || first > 0xDBFFU || index + 1 == len) {
    result = JSString::NewSingle(ctx, first);
  } else {
    const char16_t second = s->At(index + 1);
    if (second < 0xDC00U || second > 0xDFFFU) {
      result = JSString::NewSingle(ctx, first);
    } else {
      result = s->Substring(ctx, index, index + 2);
    }
  }

  iterator->Put<JSStringIterator::INDEX>(index + result->size());
  return JSIteratorResult::New(ctx, result, false);
}

// ES6
// section 21.1.5.2.2 %StringIteratorPrototype%[@@iterator]()
JSVal StringIteratorIterator(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%StringIterator%[@@iterator]", args, e);
  return args.this_binding();
}

} } }  // namespace iv::lv5::runtime
