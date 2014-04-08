#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/jsset.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsset_iterator.h>
#include <iv/lv5/jsiterator_result.h>
#include <iv/lv5/runtime/object.h>
#include <iv/lv5/runtime/set_iterator.h>
#include <iv/lv5/iterator.h>
namespace iv {
namespace lv5 {
namespace runtime {

// ES6
// section 23.2.5.2.1 %SetIteratorPrototype%.next()
JSVal SetIteratorNext(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%SetIterator%.next", args, e);
  Context* ctx = args.ctx();
  const JSVal O = args.this_binding();
  if (!O.IsObject()) {
    e->Report(Error::Type, "SetIterator next method only accepts object");
    return JSEmpty;
  }
  JSSetIterator* iterator = JSSetIterator::As(O.object());
  if (!iterator) {
    e->Report(Error::Type,
              "SetIterator next method only accepts SetIterator");
    return JSEmpty;
  }
  assert(iterator);

  JSSet::Data* s = iterator->Get<JSSetIterator::SET>();
  if (!s) {
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  const uint32_t index = iterator->Get<JSSetIterator::INDEX>();
  const SetIterationKind kind = iterator->Get<JSSetIterator::KIND>();

  if (index >= s->set().size()) {
    iterator->Put<JSSetIterator::SET>(nullptr);
    return JSIteratorResult::New(ctx, JSUndefined, true);
  }

  iterator->Put<JSSetIterator::INDEX>(index + 1);
  auto iter = s->set().begin();
  std::advance(iter, index);
  const JSVal result = *iter;
  if (kind == SetIterationKind::KEY_PLUS_VALUE) {
    return JSIteratorResult::New(
        ctx,
        JSVector::New(ctx, { result, result })->ToJSArray(),
        false);
  }
  return JSIteratorResult::New(ctx, result, false);
}

// ES6
// section 23.2.5.2.2 %SetIteratorPrototype%[@@iterator]()
JSVal SetIteratorIterator(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("%SetIterator%[@@iterator]", args, e);
  return args.this_binding();
}

} } }  // namespace iv::lv5::runtime
