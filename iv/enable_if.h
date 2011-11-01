#ifndef IV_ENABLE_IF_H_
#define IV_ENABLE_IF_H_
namespace iv {

template<bool B, class T = void>
struct enable_if_c {
  typedef T type;
};
template<class T>
struct enable_if_c<false, T> {
  // NO TYPEDEF!
};

template<class Cond, class T = void>
struct enable_if : public enable_if_c<Cond::value, T> { };

template<bool B, class T = void>
struct disable_if_c {
  typedef T type;
};

template<typename T>
struct disable_if_c<true, T> {
  // NO TYPEDEF!
};

template<class Cond, class T = void>
struct disable_if : public disable_if_c<Cond::value, T> { };


}  // namespace iv
#endif  // IV_ENABLE_IF_H_
