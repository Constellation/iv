#ifndef IV_LV5_JSSTRING_H_
#define IV_LV5_JSSTRING_H_
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsstring_fwd.h>
#include <iv/lv5/jsstring_builder.h>
namespace iv {
namespace lv5 {
namespace detail {

template<typename FiberType>
inline uint32_t SplitFiber(Context* ctx,
                           JSArray* ary,
                           const FiberType* fiber,
                           uint32_t index, uint32_t limit, Error* e) {
  ary->Reserve(fiber->size());
  for (typename FiberType::const_iterator it = fiber->begin(),
       last = fiber->end(); it != last && index != limit; ++it, ++index) {
    ary->JSArray::DefineOwnProperty(
        ctx, symbol::MakeSymbolFromIndex(index),
        DataDescriptor(
            JSString::NewSingle(ctx, *it),
            ATTR::W | ATTR::E | ATTR::C),
        false, e);
  }
  return index;
}

template<typename FiberType>
inline uint32_t SplitFiberWithOneChar(Context* ctx,
                                      JSArray* ary,
                                      uint16_t ch,
                                      JSStringBuilder* builder,
                                      const FiberType* fiber,
                                      uint32_t index,
                                      uint32_t limit, Error* e) {
  for (typename FiberType::const_iterator it = fiber->begin(),
       last = fiber->end(); it != last; ++it) {
    if (*it != ch) {
      builder->Append(*it);
    } else {
      ary->JSArray::DefineOwnProperty(
          ctx, symbol::MakeSymbolFromIndex(index),
          DataDescriptor(
              builder->Build(ctx, fiber->Is8Bit()),
              ATTR::W | ATTR::E | ATTR::C),
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


inline JSString::JSString(JSVal* src, uint32_t count)
  : size_(0),
    is_8bit_(true),
    fiber_count_(0),
    fibers_() {
  for (uint32_t i = 0; i < count; ++i) {
    this_type* str = src[i].string();
    size_ += str->size();
    fiber_count_ += str->fiber_count();
    if (!str->Is8Bit()) {
      is_8bit_ = false;
    }
  }
  if (fiber_count() > kMaxFibers) {
    Cons* cons = Cons::NewWithSize(size(), Is8Bit(), fiber_count());
    fiber_count_ = 1;
    Cons::iterator it = cons->begin();
    for (uint32_t i = 0; i < count; ++i) {
      this_type* target = src[count - 1 - i].string();
      it = std::transform(
          target->fibers().begin(),
          target->fibers().begin() + target->fiber_count(),
          it, Retainer());
    }
    fibers_[0] = cons;
  } else {
    FiberSlots::iterator it = fibers_.begin();
    for (uint32_t i = 0; i < count; ++i) {
      this_type* target = src[count - 1 - i].string();
      it = std::transform(
          target->fibers().begin(),
          target->fibers().begin() + target->fiber_count(),
          it, Retainer());
    }
  }
}

// "STRING".split("") => ['S', 'T', 'R', 'I', 'N', 'G']
JSArray* JSString::Split(Context* ctx,
                         uint32_t limit, Error* e) const {
  JSArray* ary = JSArray::New(ctx);
  if (fiber_count() == 1 && !fibers_[0]->IsCons()) {
    const FiberBase* base = static_cast<const FiberBase*>(fibers_[0]);
    if (base->Is8Bit()) {
      detail::SplitFiber(
          ctx,
          ary,
          base->As8Bit(), 0, limit, e);
    } else {
      detail::SplitFiber(
          ctx,
          ary,
          base->As16Bit(), 0, limit, e);
    }
    return ary;
  } else {
    std::vector<const FiberSlot*> slots(fibers_.begin(),
                                        fibers_.begin() + fiber_count());
    uint32_t index = 0;
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
          index = detail::SplitFiber(ctx,
                                     ary,
                                     base->As8Bit(),
                                     index, limit, e);
        } else {
          index = detail::SplitFiber(ctx,
                                     ary,
                                     base->As16Bit(),
                                     index, limit, e);
        }
        if (index == limit || slots.empty()) {
          break;
        }
      }
    }
    return ary;
  }
}

JSArray* JSString::Split(Context* ctx,
                         uint16_t ch, uint32_t limit, Error* e) const {
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
    std::vector<const FiberSlot*> slots(fibers_.begin(),
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
  ary->JSArray::DefineOwnProperty(
      ctx, symbol::MakeSymbolFromIndex(index),
      DataDescriptor(
          builder.Build(ctx),
          ATTR::W | ATTR::E | ATTR::C),
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
      return core::UStringPiece(*Get16Bit()).find(*target.Get16Bit(), index);
    }
  } else {
    if (Is8Bit()) {
      const Fiber16* rhs = target.Get16Bit();
      return core::StringPiece(*Get8Bit()).find(
          rhs->begin(), rhs->end(), index);
    } else {
      const Fiber8* rhs = target.Get8Bit();
      return core::UStringPiece(*Get16Bit()).find(
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
      return core::UStringPiece(*Get16Bit()).rfind(*target.Get16Bit(), index);
    }
  } else {
    if (Is8Bit()) {
      const Fiber16* rhs = target.Get16Bit();
      return core::StringPiece(*Get8Bit()).rfind(
          rhs->begin(), rhs->end(), index);
    } else {
      const Fiber8* rhs = target.Get8Bit();
      return core::UStringPiece(*Get16Bit()).rfind(
          rhs->begin(), rhs->end(), index);
    }
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_H_
