#ifndef IV_LV5_JSSCRIPT_H_
#define IV_LV5_JSSCRIPT_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/radio/cell.h>
namespace iv {
namespace lv5 {

class JSScript : public radio::HeapObject<radio::POINTER_CLEANUP> {
 public:
  typedef JSScript this_type;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_SCRIPT_H_
