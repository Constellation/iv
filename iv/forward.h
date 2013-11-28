#ifndef IV_FORWARD_H_
#define IV_FORWARD_H_
#include <utility>

#define IV_FORWARD_CONSTRUCTOR(Derived, Base) \
  template<class... Args> \
  Derived(Args&&... args) \
    : Base(std::forward<Args>(args)...) { }

#endif  // IV_FORWARD_H_
