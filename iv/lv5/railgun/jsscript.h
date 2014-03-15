#ifndef IV_LV5_RAILGUN_JSSCRIPT_H_
#define IV_LV5_RAILGUN_JSSCRIPT_H_
#include <iv/file_source.h>
#include <iv/lv5/jsscript.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/eval_source.h>
#include <iv/lv5/railgun/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class JSScript : public lv5::JSScript {
 public:
  virtual ~JSScript() { }

  virtual std::u16string filename() const = 0;

  void MarkChildren(radio::Core* core) { }

  virtual core::U16StringPiece SubString(std::size_t start,
                                       std::size_t len) const = 0;
};

template<typename Source>
class JSSourceScript : public JSScript {
 public:
  typedef JSSourceScript<Source> this_type;
  explicit JSSourceScript(std::shared_ptr<Source> source)
    : source_(source) {
  }

  inline std::shared_ptr<Source> source() const {
    return source_;
  }

  virtual std::u16string filename() const {
    return core::ToU16String(core::SourceTraits<Source>::GetFileName(*source_));
  }

  core::U16StringPiece SubString(std::size_t start,
                               std::size_t len) const {
    return source_->GetData().substr(start, len);
  }

  static this_type* New(Context* ctx,
                        std::shared_ptr<Source> source) {
    return new this_type(source);
  }

 private:
  std::shared_ptr<Source> source_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_JSSCRIPT_H_
