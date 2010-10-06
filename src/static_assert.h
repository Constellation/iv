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

}  // namespace iv::detail

#define STATIC_ASSERT(b)\
    typedef ::iv::detail::StaticAssertText \
        <sizeof(::iv::detail::StaticAssertFailure<(bool)(b)>)> \
        StaticAssertTypedef;

#endif  // _IV_STAITCASSERT_H_
