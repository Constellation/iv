#ifndef IV_LV5_STACK_H_
#define IV_LV5_STACK_H_
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <new>
#include <iv/detail/cstdint.h>
#include <iv/noncopyable.h>
#include <iv/singleton.h>
#include <iv/static_assert.h>
#include <iv/os_allocator.h>
#include <iv/lv5/jsval_fwd.h>
namespace iv {
namespace lv5 {

class Stack : core::Noncopyable<Stack> {
 public:
  struct EmptyTag { };

  typedef JSVal* iterator;
  typedef const JSVal* const_iterator;

  typedef std::iterator_traits<iterator>::value_type value_type;

  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  // not bytes. JSVals capacity.
  // if you calc bytes, sizeof(JSVal) * kStackCapacity
  //
  // NOTE:
  // DO NOT CHANGE THIS SIZE.
  // railgun constant register calculation is based on this size information.
  static const size_type kStackCapacity = 16 * 1024;
  static const size_type kStackBytes = kStackCapacity * sizeof(JSVal);

  IV_STATIC_ASSERT(kStackCapacity < static_cast<size_type>(INT16_MAX));

  // bytes. 4KB is page size.
  static const size_type kCommitSize = 4 * core::Size::KB;

  explicit Stack()
    : stack_(NULL),
      stack_pointer_(NULL) {
    void* ptr = core::OSAllocator::Allocate(kStackBytes);
    // core::OSAllocator::Commit(ptr, kStackBytes);
    stack_pointer_ = stack_ = reinterpret_cast<JSVal*>(ptr);
  }

  Stack(EmptyTag tag) { }  // empty

  ~Stack() {
    if (stack_) {  // normal pass
      core::OSAllocator::Decommit(stack_, kStackBytes);
      core::OSAllocator::Deallocate(stack_, kStackBytes);
    }
  }

  const_iterator begin() const {
    return stack_;
  }

  const_iterator end() const {
    return stack_ + kStackCapacity;
  }

  iterator begin() {
    return stack_;
  }

  iterator end() {
    return stack_ + kStackCapacity;
  }

  pointer GetTop() {
    return stack_pointer_;
  }

  const_pointer GetTop() const {
    return stack_pointer_;
  }

  const_pointer GetBase() const {
    return stack_;
  }

  // these 2 function Gain / Restore is reserved for ScopedArguments
  pointer Gain(size_type n) {
    if (stack_pointer_ + n < end()) {
      const pointer stack = stack_pointer_;
      stack_pointer_ += n;
      return stack;
    }
    // stack over flow
    return NULL;
  }

  pointer Gain(pointer ptr, size_type n) {
    assert(stack_ <= ptr);
    if (ptr + n < end()) {
      stack_pointer_ = ptr + n;
      return ptr;
    }
    // stack over flow
    return NULL;
  }

  void Restore(pointer ptr) {
    stack_pointer_ = ptr;
  }

  void Restore() {
    stack_pointer_ = stack_;
  }

  static std::size_t StackBaseOffset() {
    return IV_OFFSETOF(Stack, stack_);
  }

  static std::size_t StackPointerOffset() {
    return IV_OFFSETOF(Stack, stack_pointer_);
  }
 private:
  JSVal* stack_;
  JSVal* stack_pointer_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_RAILGUN_STACK_H_
