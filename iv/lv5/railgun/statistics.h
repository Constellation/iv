#ifndef IV_LV5_RAILGUN_STATISTICS_H_
#define IV_LV5_RAILGUN_STATISTICS_H_
#include <vector>
#include <utility>
#include <algorithm>
#include <iv/detail/functional.h>
#include <iv/detail/cstdint.h>
#include <iv/lv5/railgun/op.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Statistics {
 public:
  typedef std::vector<std::pair<uint32_t, OP::Type> > Histogram;

  Statistics()
    : histogram_() {
      histogram_.reserve(OP::NUM_OF_OP);
      for (int i = 0; i < OP::NUM_OF_OP; ++i) {
        histogram_.push_back(std::make_pair(0, static_cast<OP::Type>(i)));
      }
  }

  void Increment(OP::Type opcode) {
    ++(histogram_[opcode].first);
  }

  void Dump() const {
    Histogram hist = histogram_;
    std::sort(hist.begin(), hist.end(), std::greater<Histogram::value_type>());
    for (Histogram::const_iterator it = hist.begin(),
         last = hist.end(); it != last; ++it) {
      std::string str(OP::String(it->second));
      str.resize(31, ' ');
      std::cout << str << " : " << it->first << std::endl;
    }
  }

 private:
  Histogram histogram_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_STATISTICS_H_
