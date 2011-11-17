#ifndef IV_LV5_STACK_H_
#define IV_LV5_STACK_H_
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <new>
#include <iv/noncopyable.h>
#include <iv/singleton.h>
#include <iv/os_allocator.h>
#include <iv/lv5/jsval_fwd.h>
namespace iv {
namespace lv5 {

class Stack : core::Noncopyable<Stack> {
 public:
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
  static const size_type kCommitSize = 4 * core::Size::KB;

  explicit Stack(JSVal global)
    : stack_(NULL),
      stack_pointer_(NULL) {
    stack_pointer_ = stack_ =
        reinterpret_cast<JSVal*>(
            core::OSAllocator::Allocate(kStackBytes));
    *stack_ = global;
    stack_pointer_ += 1;  // for Global This
  }

  Stack() { }  // empty

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

  // these 2 function Gain / Release is reserved for ScopedArguments
  pointer Gain(size_type n) {
    if (stack_pointer_ + n < end()) {
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
  }

 protected:
  JSVal* stack_;
  JSVal* stack_pointer_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_RAILGUN_STACK_H_
