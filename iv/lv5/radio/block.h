#ifndef IV_LV5_RADIO_BLOCK_H_
#define IV_LV5_RADIO_BLOCK_H_
#include <new>
#include <iv/detail/cstdint.h>
#include <iv/utils.h>
#include <iv/debug.h>
#include <iv/noncopyable.h>
#include <iv/arith.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/block_size.h>
#include <iv/lv5/radio/block_control.h>
// this is radio::Block
// control space and memory block
namespace iv {
namespace lv5 {

class Context;

namespace radio {

class BlockControl;

class Block : private core::Noncopyable<Block> {
 public:
  typedef Block this_type;
  typedef std::size_t size_type;

  template<typename T>
  class iterator_base
    : public std::iterator<std::random_access_iterator_tag, T> {
   public:
    typedef iterator_base<T> this_type;
    typedef std::iterator<std::random_access_iterator_tag, T> super_type;
    typedef typename super_type::pointer pointer;
    typedef const typename super_type::pointer const_pointer;
    typedef typename super_type::reference reference;
    typedef const typename super_type::reference const_reference;
    typedef typename super_type::difference_type difference_type;
    typedef iterator_base<typename std::remove_const<T>::type> iterator;
    typedef iterator_base<typename std::add_const<T>::type> const_iterator;

    iterator_base(uintptr_t ptr, std::size_t size)  // NOLINT
      : ptr_(ptr), size_(size) { }

    iterator_base(const iterator& it)  // NOLINT
      : ptr_(it.ptr()), size_(it.size()) { }

    reference operator*() const {
      return *static_cast<pointer>(reinterpret_cast<void*>(ptr_));
    }
    reference operator[](difference_type d) const {
      return *static_cast<pointer>(reinterpret_cast<void*>(ptr_ + d * size_));
    }
    pointer operator->() const {
      return static_cast<pointer>(reinterpret_cast<void*>(ptr_));
    }
    this_type& operator++() {
      return ((*this) += 1);
    }
    this_type& operator++(int) {  // NOLINT
      this_type iter(*this);
      (*this)++;
      return iter;
    }
    this_type& operator--() {
      return ((*this) -= 1);
    }
    this_type& operator--(int) {  // NOLINT
      this_type iter(*this);
      (*this)--;
      return iter;
    }
    this_type& operator+=(difference_type d) {
      ptr_ = ptr_ + size_ * d;
      return *this;
    }
    this_type& operator-=(difference_type d) {
      return ((*this) += (-d));
    }

    friend bool operator==(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ == rhs.ptr_;
    }
    friend bool operator!=(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ != rhs.ptr_;
    }
    friend bool operator<(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ < rhs.ptr_;
    }
    friend bool operator<=(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ <= rhs.ptr_;
    }
    friend bool operator>(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ > rhs.ptr_;
    }
    friend bool operator>=(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ >= rhs.ptr_;
    }

    friend this_type operator+(const this_type& lhs, difference_type d) {
      this_type iter(lhs);
      return iter += d;
    }
    friend this_type operator-(const this_type& lhs, difference_type d) {
      this_type iter(lhs);
      return iter -= d;
    }
    friend difference_type operator-(const this_type& lhs,
                                     const this_type& rhs) {
      assert(lhs.size_ == rhs.size_);
      return (reinterpret_cast<uintptr_t>(lhs.ptr_) -
              reinterpret_cast<uintptr_t>(rhs.ptr_)) / lhs.size_;
    }

    uintptr_t ptr() const { return ptr_; }

    pointer Extract() const {
      return static_cast<pointer>(reinterpret_cast<void*>(ptr_));
    }

    size_type size() const { return size_; }

   private:
    uintptr_t ptr_;
    size_type size_;
  };

  typedef iterator_base<Cell> iterator;
  typedef iterator_base<const Cell> const_iterator;

  Block(size_type cell_size)
    : cell_size_(cell_size) {
    assert((reinterpret_cast<uintptr_t>(this) % kBlockSize) == 0);
    const std::size_t offset = core::math::Ceil(sizeof(this_type), cell_size_);
    start_ = reinterpret_cast<uintptr_t>(this) + offset;
    size_ = (kBlockSize - offset) / cell_size_;
  }

  iterator begin() {
    return iterator(start_, cell_size());
  }

  const_iterator begin() const {
    return const_iterator(start_, cell_size());
  }

  iterator end() {
    return iterator(start_ + cell_size() * size(), cell_size());
  }

  const_iterator end() const {
    return const_iterator(start_ + cell_size() * size(), cell_size());
  }

  bool Collect(Core* core, Context* ctx, BlockControl* control) {
    assert(IsUsed());
    bool used_cell_found = false;
    for (iterator it = begin(), last = end(); it != last; ++it) {
      Cell* cell = it.Extract();
      const Color::Type color = cell->color();
      assert(color != Color::GRAY);
      if (color == Color::WHITE) {
        control->CollectCell(core, ctx, cell);
      } else if (color == Color::BLACK) {
        cell->Coloring(Color::WHITE);
        used_cell_found = true;
      }
    }
    return used_cell_found;
  }

  // last
  void DestroyAllCells() {
    assert(IsUsed());
    for (iterator it = begin(), last = end(); it != last; ++it) {
      Cell* cell = it.Extract();
      if (cell->color() != Color::CLEAR) {
        cell->~Cell();
      }
    }
  }

  size_type size() const { return size_; }

  bool IsUsed() const { return size_ != 0; }

  void Release() {
    cell_size_ = 0;
    size_ = 0;
    start_ = 0;
  }

  size_type cell_size() const { return cell_size_; }

  void set_next(Block* block) {
    next_ = block;
  }

  Block* next() const { return next_; }

 private:
  size_type cell_size_;
  size_type size_;
  uintptr_t start_;
  Block* next_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_H_
