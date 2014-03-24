#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/jsmap.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsmap_iterator.h>
#include <iv/lv5/jsiterator_result.h>
#include <iv/lv5/runtime/object.h>
#include <iv/lv5/runtime/map_iterator.h>
namespace iv {
namespace lv5 {
namespace runtime {

// ES6
// section 23.1.5.2.1 %MapIteratorPrototype%.next()
JSVal MapIteratorNext(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%MapIterator%.next", args, e);
  Context* ctx = args.ctx();
  const JSVal O = args.this_binding();
  if (!O.IsObject()) {
    e->Report(Error::Type, "MapIterator next method only accepts object");
    return JSEmpty;
  }
  JSMapIterator* iterator = JSMapIterator::As(O.object());
  if (!iterator) {
    e->Report(Error::Type,
              "MapIterator next method only accepts MapIterator");
    return JSEmpty;
  }
  assert(iterator);

  JSMap::Data* m = iterator->Get<JSMapIterator::MAP>();
  if (!m) {
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  const uint32_t index = iterator->Get<JSMapIterator::INDEX>();
  const MapIterationKind kind = iterator->Get<JSMapIterator::KIND>();

  if (index >= m->mapping().size()) {
    iterator->Put<JSMapIterator::MAP>(nullptr);
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  iterator->Put<JSMapIterator::INDEX>(index + 1);
  auto iter = m->mapping().begin();
  std::advance(iter, index);
  JSVal result = JSUndefined;
  if (kind == MapIterationKind::KEY) {
    result = iter->first;
  } else if (kind == MapIterationKind::VALUE) {
    result = iter->second;
  } else {
    result = JSVector::New(ctx, { iter->first, iter->second })->ToJSArray();
  }
  return JSIteratorResult::New(ctx, result, false);
}

// ES6
// section 23.1.5.2.2 %MapIteratorPrototype%[@@iterator]()
JSVal MapIteratorIterator(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%MapIterator%[@@iterator]", args, e);
  return args.this_binding();
}

} } }  // namespace iv::lv5::runtime
