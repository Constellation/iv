#ifndef IV_NONCOPYABLE_H_
#define IV_NONCOPYABLE_H_

namespace iv {
namespace core {

#define IV_NONCOPYABLE(name)\
    name(const name&) = delete;\
    name& operator=(const name&) = delete;

// guard from ADL
// see http://ml.tietew.jp/cppll/cppll_novice/thread_articles/1652
namespace noncopyable_ {

template<class T = void>
class Noncopyable {
 public:
  Noncopyable() = default;
  Noncopyable(const Noncopyable&) = delete;
  const Noncopyable& operator=(const Noncopyable&) = delete;
};

}  // namespace iv::core::noncopyable_

using noncopyable_::Noncopyable;

} }  // namespace iv::core
#endif  // IV_NONCOPYABLE_H_
