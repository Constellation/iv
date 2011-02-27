#ifndef _IV_LV5_VM_STACK_H_
#define _IV_LV5_VM_STACK_H_
#include <iterator>
#include "noncopyable.h"
#include "lv5/jsval.h"
#include "lv5/os_allocator.h"
namespace iv {
namespace lv5 {

class VMStack : private core::Noncopyable<VMStack>::type {
 public:
  typedef VMStack this_type;
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
  static const size_type kStackCapacity = 16 * 1024;
  static const size_type kStackBytes = kStackCapacity * sizeof(JSVal);

  // bytes. 4KB is page size.
  static const size_type kCommitSize = 4 * 1024;
  static const size_type kMaxCallCount = 4096;

  VMStack()
    : stack_(NULL),
      stack_pointer_(NULL),
      call_count_(0),
      pages_(1) {
    stack_pointer_ = stack_ = reinterpret_cast<JSVal*>(
        OSAllocator::Allocate(kStackBytes));
  }

  ~VMStack() {
    OSAllocator::Decommit(stack_, kStackBytes);
    OSAllocator::Deallocate(stack_, kStackBytes);
  }

  pointer stack_pointer_begin() const {
    return stack_;
  }

  pointer stack_pointer_end() const {
    return stack_ + kStackCapacity;
  }

  pointer stack_pointer() const {
    return stack_pointer_;
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

  pointer Gain(size_type n) {
    if (stack_pointer_ + n < stack_pointer_end() &&
        call_count_ < kMaxCallCount) {
      ++call_count_;
      const pointer stack = stack_pointer_;
      stack_pointer_ += n;
      return stack;
    } else {
      // stack over flow
      return NULL;
    }
  }

  void Release(size_type n) {
    stack_pointer_ -= n;
    --call_count_;
  }

 private:
  pointer stack_;
  pointer stack_pointer_;
  std::size_t call_count_;
  std::size_t pages_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_VM_STACK_H_
