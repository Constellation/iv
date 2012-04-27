#ifndef IV_LV5_BIND_H_
#define IV_LV5_BIND_H_
#include <iv/stringpiece.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/attributes.h>
#include <iv/lv5/context_utils.h>
namespace iv {
namespace lv5 {

namespace bind {

class Scope {
 public:
  explicit Scope(Context* ctx) : ctx_(ctx) { }

 protected:

  Context* ctx_;
};

class Object : public Scope {
 public:
  Object(Context* ctx, JSObject* obj)
      : Scope(ctx), obj_(obj), e_() { }

  Object& cls(const Class* cls) {
    obj_->set_cls(cls);
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
          ATTR::W | ATTR::C),
      false, &e_);
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
      false, &e_);
    return *this;
  }

  Object& def(const core::StringPiece& string, const JSVal& val) {
    return def(context::Intern(ctx_, string), val);
  }

  Object& def(const Symbol& name, const JSVal& val) {
    obj_->DefineOwnProperty(
      ctx_, name,
      DataDescriptor(val, ATTR::NONE),
      false, &e_);
    return *this;
  }

  Object& def(const core::StringPiece& string, const JSVal& val, int attr) {
    return def(context::Intern(ctx_, string), val, attr);
  }

  Object& def(const Symbol& name, const JSVal& val, int attr) {
    obj_->DefineOwnProperty(
      ctx_, name,
      DataDescriptor(val, attr),
      false, &e_);
    return *this;
  }

  Object& def_accessor(const core::StringPiece& string,
                       JSObject* getter,
                       JSObject* setter, int attr) {
    return def_accessor(context::Intern(ctx_, string), getter, setter, attr);
  }

  Object& def_accessor(const Symbol& name,
                       JSObject* getter, JSObject* setter, int attr) {
    obj_->DefineOwnProperty(
      ctx_, name,
      AccessorDescriptor(getter, setter, attr),
      false, &e_);
    return *this;
  }

  Object& def_getter(const core::StringPiece& string,
                     JSObject* getter, int attr) {
    return def_accessor(context::Intern(ctx_, string), getter, NULL, attr);
  }

  Object& def_getter(const Symbol& name, JSObject* getter, int attr) {
    return def_accessor(name, getter, NULL, attr);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_getter(const core::StringPiece& string) {
    return def_getter<func, n>(context::Intern(ctx_, string));
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_getter(const Symbol& name) {
    obj_->DefineOwnProperty(
      ctx_, name,
      AccessorDescriptor(
          JSInlinedFunction<func, n>::New(ctx_, name), NULL,
          ATTR::C | ATTR::UNDEF_SETTER),
      false, &e_);
    return *this;
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_getter(const core::StringPiece& string, int attr) {
    return def_getter<func, n>(context::Intern(ctx_, string), attr);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_getter(const Symbol& name, int attr) {
    obj_->DefineOwnProperty(
      ctx_, name,
      AccessorDescriptor(
          JSInlinedFunction<func, n>::New(ctx_, name), NULL,
          attr),
      false, &e_);
    return *this;
  }

  Object& def_setter(const core::StringPiece& string,
                     JSObject* setter, int attr) {
    return def_accessor(context::Intern(ctx_, string), NULL, setter, attr);
  }

  Object& def_setter(const Symbol& name, JSObject* setter, int attr) {
    return def_accessor(name, NULL, setter, attr);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_setter(const core::StringPiece& string) {
    return def_setter<func, n>(context::Intern(ctx_, string));
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_setter(const Symbol& name) {
    obj_->DefineOwnProperty(
      ctx_, name,
      AccessorDescriptor(
          NULL, JSInlinedFunction<func, n>::New(ctx_, name),
          ATTR::C | ATTR::UNDEF_GETTER),
      false, &e_);
    return *this;
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_setter(const core::StringPiece& string, int attr) {
    return def_setter<func, n>(context::Intern(ctx_, string), attr);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  Object& def_setter(const Symbol& name, int attr) {
    obj_->DefineOwnProperty(
      ctx_, name,
      AccessorDescriptor(
          NULL, JSInlinedFunction<func, n>::New(ctx_, name),
          attr),
      false, &e_);
    return *this;
  }

  JSObject* content() const {
    return obj_;
  }

  Error* error() {
    return &e_;
  }

 private:
  JSObject* obj_;
  Error e_;
};

} } }  // namespace iv::lv5::bind
#endif  // IV_LV5_BIND_H_
