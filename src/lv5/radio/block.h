#ifndef IV_LV5_RADIO_BLOCK_H_
#define IV_LV5_RADIO_BLOCK_H_
#include <new>
#include "detail/cstdint.h"
#include "utils.h"
#include "debug.h"
#include "noncopyable.h"
#include "lv5/radio/cell.h"
#include "lv5/radio/block_size.h"
// this is radio::Block
// control space and memory block
namespace iv {
namespace lv5 {
namespace radio {

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

    iterator_base(pointer ptr, std::size_t size)  // NOLINT
      : ptr_(ptr), size_(size) { }

    iterator_base(const iterator& it)  // NOLINT
      : ptr_(it.ptr()), size_(it.size()) { }

    reference operator*() const {
      return *ptr_;
    }
    reference operator[](difference_type d) const {
      return *reinterpret_cast<pointer>(
          reinterpret_cast<uintptr_t>(ptr_) + d * size_);
    }
    pointer operator->() const {
      return ptr_;
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
      ptr_ = reinterpret_cast<pointer>(
          reinterpret_cast<uintptr_t>(ptr_) + size_ * d);
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

    pointer ptr() const { return ptr_; }

    size_type size() const { return size_; }

   private:
    pointer ptr_;
    size_type size_;
  };

  typedef iterator_base<Cell> iterator;
  typedef iterator_base<const Cell> const_iterator;

  Block(size_type cell_size)
    : cell_size_(cell_size) {
    assert((reinterpret_cast<uintptr_t>(this) % kBlockSize) == 0);
    size_ = (kBlockSize - GetControlSize()) / cell_size_;
  }

  size_type GetControlSize() const {
    return IV_ROUNDUP(sizeof(this_type), cell_size_);
  }

  iterator begin() {
    return iterator(
        (reinterpret_cast<Cell*>(
                reinterpret_cast<uintptr_t>(this) + GetControlSize())),
        cell_size());
  }

  const_iterator begin() const {
    return const_iterator(
        (reinterpret_cast<const Cell*>(
                reinterpret_cast<uintptr_t>(this) + GetControlSize())),
        cell_size());
  }

  iterator end() {
    return iterator(
        (reinterpret_cast<Cell*>(
                reinterpret_cast<uintptr_t>(this) +
                GetControlSize() + cell_size() * size())),
        cell_size());
  }

  const_iterator end() const {
    return const_iterator(
        (reinterpret_cast<const Cell*>(
                reinterpret_cast<uintptr_t>(this) +
                GetControlSize() + cell_size() * size())),
        cell_size());
  }

  template<typename Func>
  void Iterate(Func func) {
    for (iterator it = begin(), last = end(); it != last; ++it) {
      func(*it);
    }
  }

  template<typename Func>
  void Iterate(Func func) const {
    for (const_iterator it = begin(), last = end(); it != last; ++it) {
      func(*it);
    }
  }

  // destruct and chain to free list
  struct Drainer {
    Drainer(Block* block) : block_(block) { }
    void operator()(Cell& cell) {
      cell.~Cell();
      block_->Chain(&cell);
    }
    Block* block_;
  };

  void Drain() {
    Iterate(Drainer(this));
  }

  void Chain(Cell* cell) {
    cell->set_next(top_);
    top_ = cell;
  }

  void DestroyAllCells() {
  }

  size_type size() const { return size_; }

  size_type cell_size() const { return cell_size_; }

  void set_next(Block* block) {
    next_ = block;
  }

  Block* next() const { return next_; }

 private:
  size_type cell_size_;
  size_type size_;
  Cell* top_;
  Block* next_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_H_
