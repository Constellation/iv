#ifndef IV_LV5_BREAKER_STUB_ASM_H_
#define IV_LV5_BREAKER_STUB_ASM_H_
#include <iv/lv5/breaker/fwd.h>

extern "C" inline void* iv_lv5_breaker_search_exception_handler(void* pc) {
  return &iv_lv5_breaker_exception_handler_is_not_found;
}

__asm__(
  SYM(iv_lv5_breaker_dispatch_exception_handler) ":" "\n"
  "mov %rax, %rdi" "\n"
  "call " SYM(iv_lv5_breaker_search_exception_handler) "\n"
  // restore registers
  "jmpq *%rax" "\n" // jump to handler
);

__asm__(
  SYM(iv_lv5_breaker_exception_handler_is_not_found) ":" "\n"
  "ret" "\n"
);

#endif  // IV_LV5_BREAKER_STUB_ASM_H_
