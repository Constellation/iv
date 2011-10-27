#ifndef IV_LV5_RADIO_CELL_H_
#define IV_LV5_RADIO_CELL_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include "lv5/radio/color.h"
#include "lv5/radio/block_size.h"
namespace iv {
namespace lv5 {
namespace radio {

class Core;

enum CellTag {
  STRING = 0,
  OBJECT,
  REFERENCE,
  ENVIRONMENT,
  POINTER
};

class Cell {
 public:
  // next is used for free list ptr and gc mark bits
  explicit Cell(int tag) : tag_(tag), next_(Color::WHITE) { }
  Cell() : tag_(0), next_(Color::CLEAR) { }
  virtual ~Cell() { }

  int tag() const {
    return tag_;
  }

  Block* block() const {
    return reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(this) & kBlockMask);
  }

  int color() const {
    return next_ & Color::kMask;
  }

  void Coloring(Color::Type color) {
    next_ &= ~Color::kMask;  // clear color
    next_ |= color;
  }

  Cell* next() const { return reinterpret_cast<Cell*>(next_); }

  void set_next(Cell* cell) { next_ = reinterpret_cast<uintptr_t>(cell); }

  virtual void MarkChildren(Core* core) { }

 private:
  int tag_;
  uintptr_t next_;
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
