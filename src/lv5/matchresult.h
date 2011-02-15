#ifndef _IV_LV5_MATCHRESULT_H_
#define _IV_LV5_MATCHRESULT_H_
#include <tr1/tuple>
#include <tr1/cstdint>

namespace iv {
namespace lv5 {
namespace regexp {

typedef std::tr1::tuple<uint32_t, uint32_t, bool> MatchResult;
typedef std::vector<std::pair<int, int> > PairVector;

} } }  // namespace iv::lv5
#endif  // _IV_LV5_MATCHRESULT_H_
