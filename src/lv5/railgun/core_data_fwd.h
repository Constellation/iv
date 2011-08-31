#ifndef IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#define IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#include "lv5/gc_template.h"
#include "lv5/gc_kind.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/instruction_fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

class CoreData : public GCKind<CoreData> {
 public:
  typedef GCVector<Instruction>::type Data;
  typedef GCVector<Instruction*>::type InstTargets;

  static CoreData* New() {
    return new CoreData(new (GC) Data());
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

  void SetCompiled() {
    compiled_ = true;
  }

 private:
  CoreData(Data* data)
    : data_(data),
      compiled_(false) {
  }

  Data* data_;
  bool compiled_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CORE_DATA_FWD_H_
