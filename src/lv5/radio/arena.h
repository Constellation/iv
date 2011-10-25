#ifndef IV_LV5_RADIO_ARENA_H_
#define IV_LV5_RADIO_ARENA_H_
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include "detail/cstdint.h"
#include "detail/type_traits.h"
#include "utils.h"
#include "noncopyable.h"
#include "os_allocator.h"
#include "static_assert.h"
#include "lv5/radio/block.h"
namespace iv {
namespace lv5 {
namespace radio {

static const std::size_t kBlocks = 64;
static const std::size_t kArenaSize =
    IV_ALIGNED_SIZE(kBlockSize * kBlocks, kBlockSize);

// radio::Arena has 64 Blocks
class Arena : private core::Noncopyable<Arena> {
 public:
  typedef Arena this_type;
  typedef std::size_t size_type;

  // iterator
  // see http://stackoverflow.com/questions/2150192/how-to-avoid-code-duplication-implementing-const-and-non-const-iterators
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

    iterator_base(pointer ptr) : ptr_(ptr) { }  // NOLINT

    iterator_base(const iterator& it) : ptr_(it.ptr()) { }  // NOLINT

    reference operator*() const {
      return *ptr_;
    }
    reference operator[](difference_type d) const {
      return *reinterpret_cast<pointer>(
          reinterpret_cast<uintptr_t>(ptr_) + d * kBlockSize);
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
          reinterpret_cast<uintptr_t>(ptr_) + kBlockSize * d);
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
      return (reinterpret_cast<uintptr_t>(lhs.ptr_) -
              reinterpret_cast<uintptr_t>(rhs.ptr_)) / kBlockSize;
    }

    pointer ptr() const { return ptr_; }

   private:
    pointer ptr_;
  };

  typedef iterator_base<Block> iterator;
  typedef iterator_base<const Block> const_iterator;

  explicit Arena(Arena* prev)
    : next_(NULL),
      prev_(prev) {
    Initialize();
    if (prev) {
      prev->next_ = this;
    }
  }

  Arena* next() const { return next_; }

  Arena* prev() const { return prev_; }

  iterator begin() {
    return static_cast<iterator>(block_);
  }

  const_iterator begin() const {
    return const_iterator(block_);
  }

  const_iterator cbegin() const {
    return begin();
  }

  iterator end() {
    return iterator(
        reinterpret_cast<Block*>(
            reinterpret_cast<uintptr_t>(block_) + kBlockSize * kBlocks));
  }

  const_iterator end() const {
    return const_iterator(
        reinterpret_cast<const Block*>(
            reinterpret_cast<uintptr_t>(block_) + kBlockSize * kBlocks));
  }

  const_iterator cend() const {
    return end();
  }

  ~Arena() {
    core::OSAllocator::Deallocate(top_, kArenaSize);
  }

  void DestroyAllBlocks() {
    for (iterator it = begin(), last = end(); it != last; ++it) {
      it->DestroyAllCells();
    }
  }

  size_type size() const { return kBlocks; }
 private:
  void Initialize() {
    top_ = core::OSAllocator::Allocate(kArenaSize);
    block_ = reinterpret_cast<Block*>(
        IV_ALIGNED_ADDRESS(reinterpret_cast<uintptr_t>(top_), kBlockSize));
    core::OSAllocator::Commit(top_, kArenaSize);
  }

  Arena* next_;
  Arena* prev_;
  void* top_;
  Block* block_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_ARENA_H_
