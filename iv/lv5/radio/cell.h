#ifndef IV_LV5_RADIO_CELL_H_
#define IV_LV5_RADIO_CELL_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include <iv/lv5/radio/color.h>
#include <iv/lv5/radio/block_size.h>
namespace iv {
namespace lv5 {
namespace radio {

class Core;

enum CellTag {
  STRING = 0,
  OBJECT = 1,
  REFERENCE = 2,
  ENVIRONMENT = 3,
  POINTER = 4,
  POINTER_CLEANUP = 5,
  NATIVE_ITERATOR = 6
};


class Cell {
 public:
  // next is used for free list ptr and gc mark bits
  explicit Cell(int tag)
    : next_address_of_freelist_or_storage_(
        tag << Color::kOffset | Color::WHITE) {
  }
  Cell() : next_address_of_freelist_or_storage_(Color::CLEAR) { }
  virtual ~Cell() { }

  int tag() const {
    return next_address_of_freelist_or_storage_ >> Color::kOffset;
  }

  Block* block() const {
    return reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(this) & kBlockMask);
  }

  Color::Type color() const {
    return static_cast<Color::Type>(
        next_address_of_freelist_or_storage_& Color::kMask);
  }

  void Coloring(Color::Type color) {
    next_address_of_freelist_or_storage_ &= ~Color::kMask;  // clear color
    next_address_of_freelist_or_storage_ |= color;
  }

  Cell* next() const {
    return reinterpret_cast<Cell*>(next_address_of_freelist_or_storage_);
  }

  void set_next(Cell* cell) {
    next_address_of_freelist_or_storage_ = reinterpret_cast<uintptr_t>(cell);
  }

  virtual void MarkChildren(Core* core) { }

  // This is public, because offsetof to this is used in breaker::Compiler
  uintptr_t next_address_of_freelist_or_storage_;
};

template<CellTag TAG = POINTER>
class HeapObject
  : public gc,
    public Cell {
 public:
  HeapObject() : gc(), Cell(TAG) { }
};

template<>
class HeapObject<NATIVE_ITERATOR>
  : public Cell {
 public:
  HeapObject() : Cell(NATIVE_ITERATOR) { }
};

template<>
class HeapObject<STRING>
  : public gc_cleanup,
    public Cell {
 public:
  HeapObject() : gc_cleanup(), Cell(STRING) { }
};

template<>
class HeapObject<POINTER_CLEANUP>
  : public gc_cleanup,
    public Cell {
 public:
  HeapObject() : gc_cleanup(), Cell(POINTER) { }
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CELL_H_
