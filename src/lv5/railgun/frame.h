#ifndef _IV_LV5_RAILGUN_FRAME_H_
#define _IV_LV5_RAILGUN_FRAME_H_
#include <cstddef>
#include "utils.h"
#include "static_assert.h"
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

// 
// Frame structure is following
//
// FUNC | THIS | ARG1 | ARG2 | FRAME | STACK DEPTH |
//
struct Frame {
  JSVal* GetStackBase() {
    return reinterpret_cast<JSVal*>(this + 1);
  }

  JSVal* GetPreviousFrameStackTop() {
    return (reinterpret_cast<JSVal*>(this) - (argc_ + 2));
  }

  JSVal* GetFrameBase() {
    return reinterpret_cast<JSVal*>(this);
  }

  JSVal GetThis() {
    return *(reinterpret_cast<JSVal*>(this) - (argc_ + 1));
  }

  static std::size_t GetFrameSize(std::size_t n) {
    return sizeof(JSVal) * n + sizeof(Frame);
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
  std::size_t argc_;
  JSVal ret_;
};

IV_STATIC_ASSERT(AlignOf(Frame) <= AlignOf(JSVal));

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_FRAME_H_
