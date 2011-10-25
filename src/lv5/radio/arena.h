#ifndef IV_LV5_RADIO_ARENA_H_
#define IV_LV5_RADIO_ARENA_H_
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include "utils.h"
#include "noncopyable.h"
#include "os_allocator.h"
#include "lv5/radio/block.h"
namespace iv {
namespace lv5 {
namespace radio {

// radio::Arena has 64 Blocks
class Arena : private core::Noncopyable<Arena> {
  typedef Arena this_type;
  typedef std::size_t size_type;

  static const size_type kBlockSize = core::Size::KB * 4;
  static const size_type kBlocks = 64;
  static const size_type kArenaSize =
      IV_ALIGNED_SIZE(kBlockSize * kBlocks, kBlockSize);

  // iterator
  // http://episteme.wankuma.com/stlprog/_02.html
  template<typename T, typename CharPointer>
  class iterator_base
    : public std::iterator<std::random_access_iterator_tag, Block> {
   public:
    typedef iterator_base<T, CharPointer> this_type;

    iterator() : ptr_(NULL) { }
    reference operator*() const {
      return *ptr_;
    }
    reference operator[](difference_type d) const {
      reinterpret_cast<pointer>(reinterpret_cast<CharPointer>(ptr_) + d * kBlockSize);
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
          reinterpret_cast<CharPointer>(ptr_) + kBlockSize * d);
      return *this;
    }
    this_type& operator-=(difference_type d) {
      return ((*this) += (-d));
    }

    friend bool operator==(const this_type& lhs, const this_type& rhs) {
      return lhs.ptr_ == rhs.ptr_;
    }
    friend bool operator==(const this_type& lhs, const this_type& rhs) {
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
      return (reinterpret_cast<CharPointer>(lhs.ptr_) -
              reinterpret_cast<CharPointer>(rhs.ptr_)) / kBlockSize;
    }

   private:
    pointer ptr_;
  };

  typedef iterator_base<Block, char*> iterator;
  typedef iterator_base<const Block, const char*> const_iterator;

  explicit Arena(Arena* prev)
    : next_(NULL),
      prev_(prev) {
    Initialize();
    prev->next_ = this;
  }

  Arena()
    : next_(NULL),
      prev_(NULL) {
    Initialize();
  }

  iterator begin() {
    return iterator(top_);
  }

  const_iterator begin() const {
    return const_iterator(top_);
  }

  const_iterator cbegin() const {
    return begin();
  }

  iterator end() {
    return iterator(
        reinterpret_cast<Block*>(
            reinterpret_cast<Block*>(top_) + kBlockSize * kBlocks));
  }

  const_iterator end() const {
    return const_iterator(
        reinterpret_cast<const Block*>(
            reinterpret_cast<const Block*>(top_) + kBlockSize * kBlocks));
  }

  const_iterator cend() const {
    return end();
  }

  ~Arena() {
    core::OSAllocator::Deallocate(top_, kArenaSize);
  }

  size_type size() const { return kBlocks; }
 private:
  void Initialize() {
    top_ = core::OSAllocator::Allocate(kArenaSize);
    data_ = reinterpret_cast<Block*>(IV_ALIGNED_ADDRESS(top_, kBlockSize));
    core::OSAllocator::Commit(top_, kArenaSize);
  }

  Arena* next_;
  Arena* prev_;
  void* top_;
  Block* block_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_ARENA_H_
