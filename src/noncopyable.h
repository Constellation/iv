#ifndef _IV_NONCOPYABLE_H_
#define _IV_NONCOPYABLE_H_

namespace iv {
namespace core {

// guard from ADL
// see http://ml.tietew.jp/cppll/cppll_novice/thread_articles/1652
namespace noncopyable_ {

template<typename T>
class Noncopyable {
 protected:
  Noncopyable() {}
  ~Noncopyable() {}
 private:
  Noncopyable(const Noncopyable&);
  const T& operator=(const T&);
};

}  // namespace iv::core::noncopyable_

template<typename T>
struct Noncopyable {
  typedef noncopyable_::Noncopyable<T> type;
};

} }  // namespace iv::core

#endif  // _IV_NONCOPYABLE_H_
