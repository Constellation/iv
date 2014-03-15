#ifndef IV_LV5_JSSTRING_H_
#define IV_LV5_JSSTRING_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/jsstring_fwd.h>
#include <iv/lv5/jsstring_builder.h>
namespace iv {
namespace lv5 {
namespace detail {

template<typename FiberType, typename InputIterator>
inline InputIterator SplitFiber(Context* ctx,
                                InputIterator iit,
                                InputIterator ilast,
                                const FiberType* fiber) {
  for (typename FiberType::const_iterator it = fiber->begin(),
       last = fiber->end(); it != last && iit != ilast; ++it, ++iit) {
    *iit = JSString::NewSingle(ctx, *it);
  }
  return iit;
}

template<typename FiberType>
inline uint32_t SplitFiberWithOneChar(Context* ctx,
                                      JSArray* ary,
                                      char16_t ch,
                                      JSStringBuilder* builder,
                                      const FiberType* fiber,
                                      uint32_t index,
                                      uint32_t limit, Error* e) {
  for (typename FiberType::const_iterator it = fiber->begin(),
       last = fiber->end(); it != last; ++it) {
    if (*it != ch) {
      builder->Append(*it);
    } else {
      JSString* str =
          builder->Build(ctx, fiber->Is8Bit(), IV_LV5_ERROR(e));
      ary->JSArray::DefineOwnProperty(
          ctx, symbol::MakeSymbolFromIndex(index),
          DataDescriptor(str, ATTR::W | ATTR::E | ATTR::C),
          false, e);
      ++index;
      if (index == limit) {
        return index;
      }
      builder->clear();
    }
  }
  return index;
}

}  // namespace detail

inline JSString* JSString::NewSingle(Context* ctx, char16_t ch) {
  if (this_type* res = ctx->global_data()->GetSingleString(ch)) {
    return res;
  }
  return new this_type(ctx, ch);
}

inline JSString* JSString::NewEmptyString(Context* ctx) {
  return ctx->global_data()->string_empty();
}

// constructors

// empty string
inline JSString::JSString(Context* ctx)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(0),
    is_8bit_(true),
    fiber_count_(1),
    fibers_() {
  fibers_[0] = Fiber8::NewWithSize(0);
}

inline JSString::JSString(Context* ctx, const FiberBase* fiber)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(fiber->size()),
    is_8bit_(fiber->Is8Bit()),
    fiber_count_(1),
    fibers_() {
  fibers_[0] = fiber;
}

template<typename FiberType>
inline JSString::JSString(Context* ctx, const FiberType* fiber, std::size_t from, std::size_t to)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(to - from),
    is_8bit_(fiber->Is8Bit()),
    fiber_count_(1),
    fibers_() {
  fibers_[0] = FiberType::New(fiber, from, to);
}

// single char string
inline JSString::JSString(Context* ctx, char16_t ch)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(1),
    is_8bit_(core::character::IsASCII(ch)),
    fiber_count_(1),
    fibers_() {
  if (Is8Bit()) {
    fibers_[0] = Fiber8::New(&ch, 1);
  } else {
    fibers_[0] = Fiber16::New(&ch, 1);
  }
}

// external string
inline JSString::JSString(Context* ctx, const core::U16StringPiece& str)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(str.size()),
    is_8bit_(false),
    fiber_count_(1),
    fibers_() {
  fibers_[0] = Fiber16::NewWithExternal(str);
}

template<typename Iter>
inline JSString::JSString(Context* ctx, Iter it, std::size_t n, bool is_8bit)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(n),
    is_8bit_(is_8bit),
    fiber_count_(1),
    fibers_() {
  if (Is8Bit()) {
    fibers_[0] = Fiber8::New(it, size_);
  } else {
    fibers_[0] = Fiber16::New(it, size_);
  }
}

inline JSString::JSString(Context* ctx, this_type* lhs, this_type* rhs)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(lhs->size() + rhs->size()),
    is_8bit_(lhs->Is8Bit() && rhs->Is8Bit()),
    fiber_count_(lhs->fiber_count_ + rhs->fiber_count_),
    fibers_() {
  if (fiber_count() > kMaxFibers) {
    // flatten version
    Cons* cons = Cons::New(lhs, rhs);
    fiber_count_ = 1;
    fibers_[0] = cons;
    assert(Is8Bit() == fibers_[0]->Is8Bit());
  } else {
    assert(fiber_count_ <= kMaxFibers);
    // insert fibers by reverse order (rhs first)
    std::copy(
        lhs->fibers().begin(),
        lhs->fibers().begin() + lhs->fiber_count(),
        std::copy(rhs->fibers().begin(),
                  rhs->fibers().begin() + rhs->fiber_count(),
                  fibers_.begin()));
  }
}

inline JSString::JSString(Context* ctx, this_type* target, uint32_t repeat)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(target->size() * repeat),
    is_8bit_(target->Is8Bit()),
    fiber_count_(target->fiber_count() * repeat),
    fibers_() {
  if (fiber_count() > kMaxFibers) {
    Cons* cons = Cons::NewWithSize(size(), Is8Bit(), fiber_count());
    fiber_count_ = 1;
    Cons::iterator it = cons->begin();
    for (uint32_t i = 0; i < repeat; ++i) {
      it = std::copy(
          target->fibers().begin(),
          target->fibers().begin() + target->fiber_count(),
          it);
    }
    fibers_[0] = cons;
  } else {
    FiberSlots::iterator it = fibers_.begin();
    for (uint32_t i = 0; i < repeat; ++i) {
      it = std::copy(
          target->fibers().begin(),
          target->fibers().begin() + target->fiber_count(),
          it);
    }
  }
}

inline JSString::JSString(Context* ctx,
                          JSVal* src, uint32_t count,
                          size_type s, size_type fibers, bool is_8bit)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr),
    size_(s),
    is_8bit_(is_8bit),
    fiber_count_(fibers),
    fibers_() {
  if (fiber_count() > kMaxFibers) {
    Cons* cons = Cons::NewWithSize(size(), Is8Bit(), fiber_count());
    fiber_count_ = 1;
    Cons::iterator it = cons->begin();
    for (uint32_t i = 0; i < count; ++i) {
      this_type* target = src[count - 1 - i].string();
      it = std::copy(
          target->fibers().begin(),
          target->fibers().begin() + target->fiber_count(),
          it);
    }
    fibers_[0] = cons;
  } else {
    FiberSlots::iterator it = fibers_.begin();
    for (uint32_t i = 0; i < count; ++i) {
      this_type* target = src[count - 1 - i].string();
      it = std::copy(
          target->fibers().begin(),
          target->fibers().begin() + target->fiber_count(),
          it);
    }
  }
}

// "STRING".split("") => ['S', 'T', 'R', 'I', 'N', 'G']
JSArray* JSString::Split(Context* ctx,
                         uint32_t limit, Error* e) const {
  JSVector* vec = JSVector::New(ctx, std::min<uint32_t>(limit, size()));
  JSVector::iterator it = vec->begin();
  const JSVector::iterator last = vec->end();
  if (fiber_count() == 1 && !fibers_[0]->IsCons()) {
    const FiberBase* base = static_cast<const FiberBase*>(fibers_[0]);
    if (base->Is8Bit()) {
      detail::SplitFiber(ctx, it, last, base->As8Bit());
    } else {
      detail::SplitFiber(ctx, it, last, base->As16Bit());
    }
    return vec->ToJSArray();
  } else {
    trace::Vector<const FiberSlot*>::type slots(fibers_.begin(),
                                                fibers_.begin() + fiber_count());
    while (true) {
      const FiberSlot* current = slots.back();
      assert(!slots.empty());
      slots.pop_back();
      if (current->IsCons()) {
        slots.insert(slots.end(),
                     static_cast<const Cons*>(current)->begin(),
                     static_cast<const Cons*>(current)->end());
      } else {
        const FiberBase* base = static_cast<const FiberBase*>(current);
        if (base->Is8Bit()) {
          it = detail::SplitFiber(ctx, it, last, base->As8Bit());
        } else {
          it = detail::SplitFiber(ctx, it, last, base->As16Bit());
        }
        if (it == last) {
          break;
        }
      }
    }
    return vec->ToJSArray();
  }
}

JSArray* JSString::Split(Context* ctx,
                         char16_t ch, uint32_t limit, Error* e) const {
  JSStringBuilder builder;
  JSArray* ary = JSArray::New(ctx);
  uint32_t index = 0;
  if (fiber_count() == 1 && !fibers_[0]->IsCons()) {
    const FiberBase* base = static_cast<const FiberBase*>(fibers_[0]);
    if (base->Is8Bit()) {
      index = detail::SplitFiberWithOneChar(
          ctx,
          ary,
          ch,
          &builder,
          base->As8Bit(),
          0, limit, e);
    } else {
      index = detail::SplitFiberWithOneChar(
          ctx,
          ary,
          ch,
          &builder,
          base->As16Bit(),
          0, limit, e);
    }
    if (index == limit) {
      return ary;
    }
  } else {
    trace::Vector<const FiberSlot*>::type slots(fibers_.begin(),
                                                fibers_.begin() + fiber_count());
    while (true) {
      const FiberSlot* current = slots.back();
      assert(!slots.empty());
      slots.pop_back();
      if (current->IsCons()) {
        slots.insert(slots.end(),
                     static_cast<const Cons*>(current)->begin(),
                     static_cast<const Cons*>(current)->end());
      } else {
        const FiberBase* base = static_cast<const FiberBase*>(current);
        if (base->Is8Bit()) {
          index = detail::SplitFiberWithOneChar(
              ctx,
              ary,
              ch,
              &builder,
              base->As8Bit(),
              index, limit, e);
        } else {
          index = detail::SplitFiberWithOneChar(
              ctx,
              ary,
              ch,
              &builder,
              base->As16Bit(),
              index, limit, e);
        }
        if (index == limit) {
          return ary;
        }
        if (slots.empty()) {
          break;
        }
      }
    }
  }
  JSString* result = builder.Build(ctx, false, IV_LV5_ERROR(e));
  ary->JSArray::DefineOwnProperty(
      ctx, symbol::MakeSymbolFromIndex(index),
      DataDescriptor(result, ATTR::W | ATTR::E | ATTR::C),
      false, e);
  return ary;
}

JSString* JSString::Substring(Context* ctx, uint32_t from, uint32_t to) const {
  if (Is8Bit()) {
    const Fiber8* fiber = Get8Bit();
    return NewWithFiber(ctx, fiber, from, to);
  } else {
    const Fiber16* fiber = Get16Bit();
    return NewWithFiber(ctx, fiber, from, to);
  }
}

JSString::size_type JSString::find(const JSString& target,
                                   size_type index) const {
  if (Is8Bit() == target.Is8Bit()) {
    // same type
    if (Is8Bit()) {
      return core::StringPiece(*Get8Bit()).find(*target.Get8Bit(), index);
    } else {
      return core::U16StringPiece(*Get16Bit()).find(*target.Get16Bit(), index);
    }
  } else {
    if (Is8Bit()) {
      const Fiber16* rhs = target.Get16Bit();
      return core::StringPiece(*Get8Bit()).find(
          rhs->begin(), rhs->end(), index);
    } else {
      const Fiber8* rhs = target.Get8Bit();
      return core::U16StringPiece(*Get16Bit()).find(
          rhs->begin(), rhs->end(), index);
    }
  }
}

JSString::size_type JSString::rfind(const JSString& target,
                                    size_type index) const {
  if (Is8Bit() == target.Is8Bit()) {
    // same type
    if (Is8Bit()) {
      return core::StringPiece(*Get8Bit()).rfind(*target.Get8Bit(), index);
    } else {
      return core::U16StringPiece(*Get16Bit()).rfind(*target.Get16Bit(), index);
    }
  } else {
    if (Is8Bit()) {
      const Fiber16* rhs = target.Get16Bit();
      return core::StringPiece(*Get8Bit()).rfind(
          rhs->begin(), rhs->end(), index);
    } else {
      const Fiber8* rhs = target.Get8Bit();
      return core::U16StringPiece(*Get16Bit()).rfind(
          rhs->begin(), rhs->end(), index);
    }
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_H_
