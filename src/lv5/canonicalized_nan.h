#ifndef _IV_LV5_CANONICALIZED_NAN_H_
#define _IV_LV5_CANONICALIZED_NAN_H_
namespace iv {
namespace lv5 {
namespace detail {

union Trans64 {
  uint64_t bits_;
  double double_;
};

// 111111111111000000000000000000000000000000000000000000000000000
static const Trans64 kNaNTrans = { 0x7FF8000000000000LL };

}  // namespace detail

static const double kNaN = detail::kNaNTrans.double_;

} }  // namespace iv::lv5
#endif  // _IV_LV5_CANONICALIZED_NAN_H_
