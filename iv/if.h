#ifndef IV_IF_H_
#define IV_IF_H_
namespace iv {

template<bool Cond, class Then, class Else>
struct if_c {
  typedef Then type;
};

template<class Then, class Else>
struct if_c<false, Then, Else> {
  typedef Else type;
};

template<class Cond, class Then, class Else>
struct if_ : public if_c<Cond::value, Then, Else> { };

}  // namespace iv
#endif  // IV_IF_H_
