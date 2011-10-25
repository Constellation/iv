#ifndef IV_LV5_RADIO_CELL_H_
#define IV_LV5_RADIO_CELL_H_
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
  explicit Cell(int tag) : tag_(tag), next_(NULL) { }

  int tag() const {
    return tag_;
  }

  Cell* next() const { return next_; }

  void set_next(Cell* cell) { next_ = cell; }

 private:
  int tag_;
  Cell* next_;
};

template<CellTag TAG = POINTER>
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
#endif  // IV_LV5_RADIO_CELL_H_
