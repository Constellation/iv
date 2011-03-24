#ifndef _IV_LV5_BIND_H_
#define _IV_LV5_BIND_H_
#include "stringpiece.h"
#include "lv5/jsobject.h"
#include "lv5/context.h"
namespace iv {
namespace lv5 {
namespace bind {

enum Attribute {
  NONE = PropertyDescriptor::NONE,
  WRITABLE = PropertyDescriptor::WRITABLE,
  ENUMERABLE = PropertyDescriptor::ENUMERABLE,
  CONFIGURABLE = PropertyDescriptor::CONFIGURABLE,
  N = NONE,
  W = WRITABLE,
  E = ENUMERABLE,
  C = CONFIGURABLE
};

class Scope {
 public:
  Scope(Context* ctx) : ctx_(ctx) { }

 protected:

  Context* ctx_;
};

//class Definition {
//};
//
//template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
//class Method : public Definition {
//};
//
//class Object : public Scope {
// public:
//  Object(Context* ctx) : Scope(ctx) { }
//
//  Object& operator[](Definition def) {
//    return *this;
//  }
//};

class Object : public Scope {
 public:
  Object(Context* ctx, JSObject* obj)
      : Scope(ctx), obj_(obj) { }

  Object& class_name(const core::StringPiece& string) {
    return class_name(context::Intern(ctx_, string));
  }

  Object& class_name(const Symbol& name) {
    obj_->set_class_name(name);
    return *this;
  }

  Object& prototype(JSObject* proto) {
    obj_->set_prototype(proto);
    return *this;
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def(const core::StringPiece& string) {
    return def<func, n>(context::Intern(ctx_, string));
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def(const Symbol& name) {
    obj_->DefineOwnProperty(
      ctx_, name,
      DataDescriptor(
          JSInlinedFunction<func, n>::New(ctx_, name),
          WRITABLE | CONFIGURABLE),
      false, ctx_->error());
    return *this;
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def(const core::StringPiece& string, int attr) {
    return def<func, n>(context::Intern(ctx_, string), attr);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def(const Symbol& name, int attr) {
    obj_->DefineOwnProperty(
      ctx_, name,
      DataDescriptor(
          JSInlinedFunction<func, n>::New(ctx_, name),
          attr),
      false, ctx_->error());
    return *this;
  }

  Object& def(const core::StringPiece& string, const JSVal& val) {
    return def(context::Intern(ctx_, string), val);
  }

  Object& def(const Symbol& name, const JSVal& val) {
    obj_->DefineOwnProperty(
      ctx_, name,
      DataDescriptor(val, NONE),
      false, ctx_->error());
    return *this;
  }

  Object& def(const core::StringPiece& string, const JSVal& val, int attr) {
    return def(context::Intern(ctx_, string), val, attr);
  }

  Object& def(const Symbol& name, const JSVal& val, int attr) {
    obj_->DefineOwnProperty(
      ctx_, name,
      DataDescriptor(val, attr),
      false, ctx_->error());
    return *this;
  }

 private:
  JSObject* obj_;
};

} } }  // namespace iv::lv5::bind
#endif  // _IV_LV5_BIND_H_
