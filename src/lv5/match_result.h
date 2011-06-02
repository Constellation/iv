#ifndef _IV_LV5_MATCH_RESULT_H_
#define _IV_LV5_MATCH_RESULT_H_
#include "detail/tr1/tuple.h"
#include "detail/tr1/cstdint.h"

namespace iv {
namespace lv5 {
namespace regexp {

typedef std::tr1::tuple<uint32_t, uint32_t, bool> MatchResult;
typedef std::vector<std::pair<int, int> > PairVector;

} } }  // namespace iv::lv5
#endif  // _IV_LV5_MATCH_RESULT_H_
