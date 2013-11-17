#ifndef IV_LV5_JSOBJECT_WITH_TUPLE_H_
#define IV_LV5_JSOBJECT_WITH_TUPLE_H_
#include <tuple>
#include <iv/detail/cstdint.h>
#include <iv/lv5/jsobject.h>
namespace iv {
namespace lv5 {

class JSObjectTuple : public JSObject {
 public:
  uintptr_t unique() const { return unique_; }

 protected:
  template<class... Args>
  JSObjectTuple(uintptr_t unique, Args... args)
    : JSObject(args...)
    , unique_(unique) {
    MakeTuple();
    set_cls(GetClass());
  }

 private:
  uintptr_t unique_;
};

template<class... Args>
class JSObjectWithTuple : public JSObjectTuple {
 public:
  typedef std::tuple<Args...> Tuple;

  static JSObjectWithTuple<Args...>* New(Context* ctx,
                                         Map* map,
                                         uintptr_t unique, Args&&... args) {
    return new JSObjectWithTuple(ctx, map, unique, std::forward<Args>(args)...);
  }

  template<std::size_t N>
  typename std::tuple_element<N, Tuple>::type Get() {
    return std::get<N>(tuple_);
  }

  template<std::size_t N>
  void Put(typename std::tuple_element<N, Tuple>::type value) {
    std::get<N>(tuple_) = value;
  }

  virtual void MarkChildren(radio::Core* core) {
  }

 protected:
  JSObjectWithTuple(Context* ctx, Map* map, uintptr_t unique, Args... args)
    : JSObjectTuple(unique, map)
    , tuple_(args...) {
  }

  Tuple tuple_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_WITH_TUPLE_H_
