// JSPrimitive allows string, number, boolean, undefined, null, empty.
// Not allow object or something.
//
// But basically, empty is not allowed in operation.
#ifndef IV_LV5_JSPRIMITIVE_H_
#define IV_LV5_JSPRIMITIVE_H_
#include <iv/lv5/jsval_fwd.h>
namespace iv {
namespace lv5 {

class JSPrimitive : public JSLayout {
 public:
  typedef JSLayout generic_type;
  typedef JSPrimitive this_type;

  JSPrimitive() {
    set_undefined();
  }

  JSPrimitive(double val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsNumber());
  }

  JSPrimitive(JSString* val)  // NOLINT
    : JSLayout() {
    set_value(val);
  }

  JSPrimitive(detail::JSTrueType val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsBoolean());
  }

  JSPrimitive(detail::JSFalseType val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsBoolean());
  }

  JSPrimitive(detail::JSNullType val)  // NOLINT
    : JSLayout() {
    set_null();
    assert(IsNull());
  }

  JSPrimitive(detail::JSUndefinedType val)  // NOLINT
    : JSLayout() {
    set_undefined();
    assert(IsUndefined());
  }

  JSPrimitive(detail::JSEmptyType val)  // NOLINT
    : JSLayout() {
    set_empty();
    assert(IsEmpty());
  }

  JSPrimitive(detail::JSNaNType val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsNumber());
  }

  explicit JSPrimitive(JSLayout layout)
    : JSLayout(layout) {
    assert(IsPrimitive() || IsNullOrUndefined() || IsEmpty());
  }

  JSString* ToString(Context* ctx) const {
    Error::Dummy dummy;
    return generic_type::ToString(ctx, &dummy);
  }

  double ToNumber(Context* ctx) const {
    Error::Dummy dummy;
    return generic_type::ToNumber(ctx, &dummy);
  }

  static bool IsConversible(JSVal value) {
    return value.IsNullOrUndefined() || value.IsPrimitive() || value.IsEmpty();
  }

  template<bool LeftFirst>
  static CompareResult Compare(Context* ctx, this_type lhs, this_type rhs) {
    Error::Dummy dummy;
    return JSVal::Compare<LeftFirst>(ctx, lhs, rhs, &dummy);
  }

  static bool AbstractEqual(Context* ctx, this_type lhs, this_type rhs) {
    Error::Dummy dummy;
    return JSVal::AbstractEqual(ctx, lhs, rhs, &dummy);
  }

 private:
  operator JSVal() {
    return *this;
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSPRIMITIVE_H_
