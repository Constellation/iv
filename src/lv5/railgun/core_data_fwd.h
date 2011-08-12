#ifndef _IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#define _IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#include "lv5/gc_template.h"
#include "lv5/gc_hook.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/instruction_fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

class CoreData : public GCHook<CoreData> {
 public:
  typedef GCVector<Instruction>::type Data;
  typedef GCVector<Instruction*>::type InstTargets;

  static CoreData* New() {
    Data* data = new(GC)Data();
    InstTargets* targets = new(GC)InstTargets();
    return new CoreData(data, targets);
  }

  inline GC_ms_entry* MarkChildren(GC_word* top,
                                   GC_ms_entry* entry,
                                   GC_ms_entry* mark_sp_limit,
                                   GC_word env);

  Data* data() {
    return data_;
  }

  const Data* data() const {
    return data_;
  }

  InstTargets* targets() {
    return targets_;
  }

  const InstTargets* targets() const {
    return targets_;
  }

 private:
  CoreData(Data* data, InstTargets* targets)
    : data_(data),
      targets_(targets) {
  }

  Data* data_;
  InstTargets* targets_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CORE_DATA_FWD_H_
