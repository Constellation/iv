#ifndef IV_LV5_HEAP_OBJECT_H_
#define IV_LV5_HEAP_OBJECT_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
namespace iv {
namespace lv5 {
namespace radio {

enum CellTag {
  STRING = 0,
  OBJECT,
  REFERENCE,
  ENVIRONMENT,
  POINTER
};

class Cell {
 public:
  Cell(int tag) : tag_(tag) { }

  int tag() const {
    return tag_;
  }

 private:
  int tag_;
};

template<CellTag TAG>
class HeapObject
  : public gc,
    public Cell {
 public:
  HeapObject() : gc(), Cell(TAG) { }
};

template<>
class HeapObject<STRING>
  : public gc_cleanup,
    public Cell {
 public:
  HeapObject() : gc(), Cell(STRING) { }
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_HEAP_OBJECT_H_
