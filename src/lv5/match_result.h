#ifndef IV_LV5_MATCH_RESULT_H_
#define IV_LV5_MATCH_RESULT_H_
#include "detail/tuple.h"
#include "detail/cstdint.h"

namespace iv {
namespace lv5 {
namespace regexp {

typedef std::tuple<uint32_t, uint32_t, bool> MatchResult;
typedef std::vector<std::pair<int, int> > PairVector;

} } }  // namespace iv::lv5
#endif  // IV_LV5_MATCH_RESULT_H_
