#ifndef _IV_LV5_RAILGUN_FRAME_H_
#define _IV_LV5_RAILGUN_FRAME_H_
#include <cstddef>
#include "utils.h"
#include "static_assert.h"
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/code.h"
namespace iv {
namespace lv5 {
namespace railgun {

#define IV_ROUNDUP(x, y)\
  (((x) + (y - 1)) & ~(y - 1))

//
// Frame structure is following
//
// FUNC | THIS | ARG1 | ARG2 | FRAME | STACK DEPTH |
//
struct Frame {
  JSVal* GetFrameEnd() {
    return GetFrameBase() + GetFrameSize(code_->stack_depth());
  }

  JSVal* GetStackBase() {
    return reinterpret_cast<JSVal*>(this) + (IV_ROUNDUP(sizeof(Frame), sizeof(JSVal)) / sizeof(JSVal));
  }

  JSVal* GetPreviousFrameStackTop() {
    return (reinterpret_cast<JSVal*>(this) - (argc_ + 2));
  }

  JSVal* GetFrameBase() {
    return reinterpret_cast<JSVal*>(this);
  }

  JSVal GetThis() const {
    return this_binding();
  }

  JSVal this_binding() const {
    return *(reinterpret_cast<const JSVal*>(this) - (argc_ + 1));
  }

  void set_this_binding(JSVal val) {
    *(reinterpret_cast<JSVal*>(this) - (argc_ + 1)) = val;
  }

  JSVal* arguments_begin() {
    return reinterpret_cast<JSVal*>(this) - argc_;
  }

  JSVal* arguments_end() {
    return GetFrameBase();
  }

  static std::size_t GetFrameSize(std::size_t n) {
    return n + (IV_ROUNDUP(sizeof(Frame), sizeof(JSVal)) / sizeof(JSVal));
  }

  const Code* code() const {
    return code_;
  }

  const uint8_t* data() const {
    return code_->data();
  }

  JSVal* stacktop() {
    return GetStackBase();
  }

  JSEnv* lexical_env() {
    return lexical_env_;
  }

  void set_lexical_env(JSEnv* lex) {
    lexical_env_ = lex;
  }

  JSEnv* variable_env() {
    return variable_env_;
  }

  const JSVals& constants() const {
    return code_->constants();
  }

  Code* code_;
  const uint8_t* prev_pc_;
  JSEnv* variable_env_;
  JSEnv* lexical_env_;
  Frame* prev_;
  uint16_t argc_;
  uint16_t dynamic_env_level_;
  JSVal ret_;
};

#undef IV_ROUNDUP

IV_STATIC_ASSERT(AlignOf(Frame) <= AlignOf(JSVal));

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_FRAME_H_
