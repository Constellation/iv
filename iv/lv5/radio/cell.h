#ifndef IV_LV5_RADIO_CELL_H_
#define IV_LV5_RADIO_CELL_H_
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include <iv/byteorder.h>
#include <iv/lv5/radio/color.h>
#include <iv/lv5/radio/block_size.h>
namespace iv {
namespace lv5 {

class Map;

namespace radio {

class Core;

enum CellTag {
  STRING = 0,
  OBJECT = 1,
  SYMBOL = 2,
  REFERENCE = 3,
  ENVIRONMENT = 4,
  POINTER = 5,
  POINTER_CLEANUP = 6,
  NATIVE_ITERATOR = 7,
  ACCESSOR = 8
};


class Cell {
 public:
  // next is used for free list ptr and gc mark bits
  union Tag {
#if defined(IV_IS_LITTLE_ENDIAN)
    struct Pair {
      uint32_t color;
      uint32_t tag;
    } pair;
#else
    struct Pair {
      uint32_t tag;
      uint32_t color;
    } pair;
#endif
    uint64_t next_address_of_freelist;
  };

  static const int kTagOffset = offsetof(Tag, pair) + offsetof(Tag::Pair, tag);

  static int TagOffset() {
    return IV_OFFSETOF(Cell, storage_) + kTagOffset;
  }

  explicit Cell(uint32_t tag) {
    storage_.pair.tag = tag;
    storage_.pair.color = Color::WHITE;
  }

  Cell() {
    storage_.pair.tag = 0;
    storage_.pair.color = Color::CLEAR;
  }

  virtual ~Cell() { }

  uint32_t tag() const {
    return storage_.pair.tag;
  }

  Block* block() const {
    return reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(this) & kBlockMask);
  }

  Color::Type color() const {
    return static_cast<Color::Type>(storage_.pair.color & Color::kMask);
  }

  void Coloring(Color::Type color) {
    storage_.pair.color = color;
  }

  Cell* next() const {
    return reinterpret_cast<Cell*>(storage_.next_address_of_freelist);
  }

  void set_next(Cell* cell) {
    storage_.next_address_of_freelist = reinterpret_cast<uint64_t>(cell);
  }

  virtual void MarkChildren(Core* core) { }

  // This is public, because offsetof to this is used in breaker::Compiler
  Tag storage_;
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
  : public gc,
    public Cell {
 public:
  HeapObject() : gc(), Cell(STRING) { }
};

template<>
class HeapObject<NATIVE_ITERATOR>
  : public Cell {
 public:
  HeapObject() : Cell(NATIVE_ITERATOR) { }
};

template<>
class HeapObject<POINTER_CLEANUP>
  : public gc_cleanup,
    public Cell {
 public:
  HeapObject() : gc_cleanup(), Cell(POINTER) { }
};

class CellObject : public gc, public Cell {
 public:
  CellObject(CellTag tag) : gc(), Cell(tag) { }
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CELL_H_
