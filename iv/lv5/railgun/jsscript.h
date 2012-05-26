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
  typedef JSScript this_type;
  enum Type {
    kGlobal,
    kEval,
    kFunction
  };

  virtual ~JSScript() { }

  virtual Type type() const = 0;

  virtual core::UString filename() const = 0;

  void MarkChildren(radio::Core* core) { }

  virtual core::UStringPiece SubString(std::size_t start,
                                       std::size_t len) const = 0;
};

template<typename Source>
class JSEvalScript : public JSScript {
 public:
  typedef JSEvalScript<Source> this_type;
  explicit JSEvalScript(std::shared_ptr<Source> source)
    : source_(source) {
  }

  inline Type type() const {
    return kEval;
  }

  inline std::shared_ptr<Source> source() const {
    return source_;
  }

  virtual core::UString filename() const {
    return core::ToUString("[eval]");
  }

  core::UStringPiece SubString(std::size_t start,
                               std::size_t len) const {
    return source_->GetData().substr(start, len);
  }

  static this_type* New(Context* ctx,
                        std::shared_ptr<Source> source) {
    return new JSEvalScript<Source>(source);
  }

 private:
  std::shared_ptr<Source> source_;
};



class JSGlobalScript : public JSScript {
 public:
  typedef JSGlobalScript this_type;
  explicit JSGlobalScript(const core::FileSource* source)
    : source_(source) {
  }

  inline Type type() const {
    return kGlobal;
  }

  inline const core::FileSource* source() const {
    return source_;
  }

  virtual core::UString filename() const {
    return core::ToUString(
        core::SourceTraits<core::FileSource>::GetFileName(*source_));
  }

  core::UStringPiece SubString(std::size_t start,
                               std::size_t len) const {
    return source_->GetData().substr(start, len);
  }

  static this_type* New(Context* ctx, const core::FileSource* source) {
    return new JSGlobalScript(source);
  }

 private:
  const core::FileSource* source_;
};


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_JSSCRIPT_H_
