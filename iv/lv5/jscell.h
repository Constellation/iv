#ifndef IV_LV5_JSCELL_H_
#define IV_LV5_JSCELL_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {

class JSCell : public radio::CellObject {
 public:
  JSCell(radio::CellTag tag, Map* map) : radio::CellObject(tag), map_(map) { }

  Map* map() const { return map_; }
  void set_map(Map* map) { map_ = map; }

  static std::size_t MapOffset() { return IV_OFFSETOF(JSCell, map_); }

  // implementation in jsobject.h
  virtual void MarkChildren(radio::Core* core);

 private:
  Map* map_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSCELL_H_
