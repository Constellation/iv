#ifndef IV_LV5_JSCELL_H_
#define IV_LV5_JSCELL_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSCell : public radio::CellObject {
 public:
  JSCell(radio::CellTag tag, Map* map, const Class* cls) : radio::CellObject(tag), map_(map), cls_(cls) { }

  Map* map() const { return map_; }
  void set_map(Map* map) { map_ = map; }

  static std::size_t ClassOffset() { return IV_OFFSETOF(JSCell, cls_); }
  static std::size_t MapOffset() { return IV_OFFSETOF(JSCell, map_); }

  // implementation in jsobject.h
  Map* FlattenMap() const;

  // implementation in jsobject.h
  JSObject* prototype() const;

  // implementation in jsobject.h
  virtual void MarkChildren(radio::Core* core);

  const Class* cls() const {
    return cls_;
  }

  void set_cls(const Class* cls) {
    cls_ = cls;
  }

  template<Class::JSClassType CLS>
  bool IsClass() const {
    return cls_->type == static_cast<uint32_t>(CLS);
  }

  bool IsClass(Class::JSClassType cls) const {
    return cls_->type == static_cast<uint32_t>(cls);
  }

 private:
  Map* map_;
  const Class* cls_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSCELL_H_
