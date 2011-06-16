// railgun vm stack
// construct Frame on this stack,
// and traverse Frames when GC maker comes
#ifndef _IV_LV5_RAILGUN_STACK_H_
#define _IV_LV5_RAILGUN_STACK_H_
#include "noncopyable.h"
#include "os_allocator.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Stack : core::Noncopyable<Stack> {
 public:
  // returns new frame for function call
  Frame* NewFrame() {
  }

 private:
  void* stack_base_;
  void* stack_end_;
  void* stack_pointer_;
  std::size_t call_count_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_STACK_H_
