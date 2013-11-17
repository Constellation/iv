#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsarray_iterator.h>
#include <iv/lv5/jsiterator_result.h>
#include <iv/lv5/runtime/object.h>
#include <iv/lv5/runtime/array_iterator.h>
namespace iv {
namespace lv5 {
namespace runtime {

// ES6
// section 22.1.5.2.1 %ArrayIteratorPrototype%.next()
JSVal ArrayIteratorNext(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%ArrayIterator%.next", args, e);
  Context* ctx = args.ctx();
  const JSVal O = args.this_binding();
  if (!O.IsObject()) {
    e->Report(Error::Type, "ArrayIterator next method only accepts object");
    return JSEmpty;
  }
  JSArrayIterator* iterator = JSArrayIterator::As(O.object());
  if (!iterator) {
    e->Report(Error::Type,
              "ArrayIterator next method only accepts ArrayIterator");
    return JSEmpty;
  }
  assert(iterator);

  JSObject* a = iterator->Get<JSArrayIterator::ITERATED>();
  if (!a) {
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  const uint32_t index = iterator->Get<JSArrayIterator::INDEX>();
  const ArrayIterationKind kind = iterator->Get<JSArrayIterator::KIND>();
  const uint32_t len = internal::GetLength(ctx, a, IV_LV5_ERROR(e));

  if (index >= len) {
    iterator->Put<JSArrayIterator::ITERATED>(nullptr);
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  iterator->Put<JSArrayIterator::INDEX>(index + 1);

  if (kind == ArrayIterationKind::KEY) {
    return JSIteratorResult::New(ctx, JSVal::UInt32(index), false);
  }

  const JSVal value =
      a->Get(ctx, symbol::MakeSymbolFromIndex(index), IV_LV5_ERROR(e));
  switch (kind) {
    case ArrayIterationKind::KEY_PLUS_VALUE:
    case ArrayIterationKind::SPARSE_KEY_PLUS_VALUE: {
      JSVector* vec = JSVector::New(ctx, 2);
      (*vec)[0] = JSVal::UInt32(index);
      (*vec)[1] = value;
      return JSIteratorResult::New(ctx, vec->ToJSArray(), false);
    }

    default:
      return JSIteratorResult::New(ctx, value, false);
  }
}

// ES6
// section 22.1.5.2.2 %ArrayIteratorPrototype%[@@iterator]()
JSVal ArrayIteratorIterator(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%ArrayIterator%[@@iterator]", args, e);
  return args.this_binding();
}

} } }  // namespace iv::lv5::runtime
