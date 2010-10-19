#ifndef _IV_STATICASSERT_H
#define _IV_STATICASSERT_H_
namespace iv {
namespace detail {
template<bool b>
struct StaticAssertFailure;

template<>
struct StaticAssertFailure<true> {
  enum {
    value = 1
  };
};

template<int x>
struct StaticAssertTest {
};

} }  // namespace iv::detail

#define IV_STATIC_ASSERT(b)\
  typedef ::iv::detail::StaticAssertTest \
    <sizeof(::iv::detail::StaticAssertFailure<static_cast<bool>(b)>)> \
        StaticAssertTypedef##__LINE__;

#endif  // _IV_STAITCASSERT_H_
