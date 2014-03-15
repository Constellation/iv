#ifndef IV_LV5_RAILGUN_CONSTANT_POOL_H_
#define IV_LV5_RAILGUN_CONSTANT_POOL_H_
#include <iv/detail/cstdint.h>
#include <iv/detail/unordered_map.h>
#include <iv/detail/functional.h>
#include <iv/ustring.h>
#include <iv/utils.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsstring_fwd.h>
#include <iv/lv5/error.h>
#include <iv/lv5/railgun/context_fwd.h>
#include <iv/lv5/railgun/code.h>
namespace iv {
namespace lv5 {
namespace railgun {

class ConstantPool {
  // Bytecode register size is very limited.
  // So probably, constant register may be overflown sometimes.
  // When constant register is overflown, we can use LOAD_CONST instead of
  // constant register.
 public:
  static const uint32_t kEmpty = UINT32_MAX;

  typedef std::unordered_map<core::UString, int32_t> JSStringToIndexMap;
  typedef std::unordered_map<
      double,
      int32_t,
      std::hash<double>, JSDoubleEquals> JSDoubleToIndexMap;

  ConstantPool(Context* ctx)
    : ctx_(ctx),
      code_(nullptr),
      jsstring_to_index_map_(),
      double_to_index_map_(),
      undefined_index_(),
      null_index_(),
      true_index_(),
      false_index_(),
      empty_index_() {
  }

  void Init(Code* code) {
    code_ = code;
    jsstring_to_index_map_.clear();
    double_to_index_map_.clear();
    undefined_index_ = kEmpty;
    null_index_ = kEmpty;
    true_index_ = kEmpty;
    false_index_ = kEmpty;
    empty_index_ = kEmpty;
  }

  uint32_t undefined_index() {
    if (undefined_index_ != kEmpty) {
      return undefined_index_;
    }
    undefined_index_ = AddConstant(JSUndefined);
    return undefined_index_;
  }

  uint32_t null_index() {
    if (null_index_ != kEmpty) {
      return null_index_;
    }
    null_index_ = AddConstant(JSNull);
    return null_index_;
  }

  uint32_t true_index() {
    if (true_index_ != kEmpty) {
      return true_index_;
    }
    true_index_ = AddConstant(JSTrue);
    return true_index_;
  }

  uint32_t false_index() {
    if (false_index_ != kEmpty) {
      return false_index_;
    }
    false_index_ = AddConstant(JSFalse);
    return false_index_;
  }

  uint32_t empty_index() {
    if (empty_index_ != kEmpty) {
      return empty_index_;
    }
    empty_index_ = AddConstant(JSEmpty);
    return empty_index_;
  }

  uint32_t string_index(const core::UString& str) {
    const JSStringToIndexMap::const_iterator it =
        jsstring_to_index_map_.find(str);

    if (it != jsstring_to_index_map_.end()) {
      // duplicate constant
      return it->second;
    }

    // new constant value
    Error::Dummy dummy;
    const uint32_t index = AddConstant(
        JSString::New(
            ctx_,
            str.begin(),
            str.end(),
            core::character::IsASCII(str.begin(), str.end()), &dummy));
    jsstring_to_index_map_.insert(std::make_pair(str, index));
    return index;
  }

  uint32_t string_index(const StringLiteral* str) {
    return string_index(core::ToUString(str->value()));
  }

  uint32_t string_index(const core::string_view& str) {
    return string_index(core::ToUString(str));
  }

  uint32_t string_index(const core::u16string_view& str) {
    return string_index(core::UString(str));
  }

  uint32_t number_index(double val) {
    const JSDoubleToIndexMap::const_iterator it =
        double_to_index_map_.find(val);

    if (it != double_to_index_map_.end()) {
      // duplicate constant pool
      return it->second;
    }

    // new constant value
    const uint32_t index = AddConstant(val);
    double_to_index_map_.insert(std::make_pair(val, index));
    return index;
  }

  uint32_t Lookup(JSVal constant) {
    if (constant.IsString()) {
      return string_index(constant.string()->GetUString());
    } else if (constant.IsNumber()) {
      return number_index(constant.number());
    } else if (constant.IsUndefined()) {
      return undefined_index();
    } else if (constant.IsNull()) {
      return null_index();
    } else if (constant.IsBoolean()) {
      return (constant.boolean()) ? true_index() : false_index();
    } else if (constant.IsEmpty()) {
      return empty_index();
    }
    UNREACHABLE();
    return 0;  // makes compiler happy
  }

 private:
  uint32_t AddConstant(JSVal value) {
    const uint32_t result = code_->constants().size();
    code_->constants_.push_back(value);
    return result;
  }

  Context* ctx_;
  Code* code_;
  JSStringToIndexMap jsstring_to_index_map_;
  JSDoubleToIndexMap double_to_index_map_;
  uint32_t undefined_index_;
  uint32_t null_index_;
  uint32_t true_index_;
  uint32_t false_index_;
  uint32_t empty_index_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONSTANT_POOL_H_
