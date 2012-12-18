#ifndef IV_LV5_ACCESSOR_H_
#define IV_LV5_ACCESSOR_H_
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/context_fwd.h>
namespace iv {
namespace lv5 {

class Accessor : public radio::HeapObject<radio::ACCESSOR> {
 public:
  Accessor(JSObject* getter, JSObject* setter)
    : getter_(getter),
      setter_(setter) {
  }

  static Accessor* New(Context* ctx, JSObject* getter, JSObject* setter) {
    return new Accessor(getter, setter);
  }

  JSObject* getter() const { return getter_; }
  void set_getter(JSObject* obj) { getter_ = obj; }
  JSObject* setter() const { return setter_; }
  void set_setter(JSObject* obj) { setter_ = obj; }

  JSVal InvokeGetter(Context* ctx, JSVal this_binding, Error* e) {
    if (getter()) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return static_cast<JSFunction*>(getter())->Call(&a, this_binding, e);
    } else {
      return JSUndefined;
    }
  }

  virtual void MarkChildren(radio::Core* core) {
    core->MarkCell(getter_);
    core->MarkCell(setter_);
  }

  static std::size_t GetterOffset() {
    return IV_OFFSETOF(Accessor, getter_);
  }

  static std::size_t SetterOffset() {
    return IV_OFFSETOF(Accessor, setter_);
  }
 private:
  JSObject* getter_;
  JSObject* setter_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ACCESSOR_H_
