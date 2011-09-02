#ifndef IV_CALLONCE_POSIX_H_
#define IV_CALLONCE_POSIX_H_
#include <pthread.h>
namespace iv {
namespace core {
namespace thread {

#define IV_ONCE_INIT PTHREAD_ONCE_INIT

typedef pthread_once_t Once;

inline void CallOnce(Once* once, OnceCallback func) {
  pthread_once(once, func);
}

} } }  // iv::core::thread
#endif  // IV_CALLONCE_POSIX_H_
