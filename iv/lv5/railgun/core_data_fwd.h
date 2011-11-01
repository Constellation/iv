#ifndef IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#define IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#include <iv/lv5/gc_template.h>
#include <iv/lv5/gc_kind.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/instruction_fwd.h>
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

  GC_ms_entry* MarkChildren(GC_word* top,
                            GC_ms_entry* entry,
                            GC_ms_entry* mark_sp_limit,
                            GC_word env);

  void MarkChildren(radio::Core* core);

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
  explicit CoreData(Data* data)
    : data_(data),
      compiled_(false) {
  }

  Data* data_;
  bool compiled_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CORE_DATA_FWD_H_
