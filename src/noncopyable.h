#ifndef _IV_NONCOPYABLE_H_
#define _IV_NONCOPYABLE_H_

namespace iv {
namespace core {

// guard from ADL
// see http://ml.tietew.jp/cppll/cppll_novice/thread_articles/1652
namespace noncopyable_ {

class Noncopyable {
 protected:
  Noncopyable() {}
  ~Noncopyable() {}
 private:
  Noncopyable(const Noncopyable&);
  const Noncopyable& operator=(const Noncopyable&);
};

}  // namespace iv::core::noncopyable_

typedef noncopyable_::Noncopyable Noncopyable;

} }  // namespace iv::core

#endif  // _IV_NONCOPYABLE_H_

