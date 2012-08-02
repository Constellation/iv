#ifndef IV_LV5_ACCESSOR_H_
#define IV_LV5_ACCESSOR_H_
namespace iv {
namespace lv5 {

class Accessor : public radio::HeapObject<radio::OBJECT> {
 public:
  JSObject* getter;
  JSObject* setter;

  virtual void MarkChildren(Core* core) {
    core->MarkCell(getter);
    core->MarkCell(setter);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ACCESSOR_H_
