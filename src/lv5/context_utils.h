#ifndef _IV_LV5_CONTEXT_UTILS_H_
#define _IV_LV5_CONTEXT_UTILS_H_
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "lv5/class.h"
#include "lv5/symbol.h"
#include "lv5/specialized_ast.h"
namespace iv {
namespace lv5 {

class JSVal;
class JSString;
class Context;
class JSRegExpImpl;
class JSFunction;
class GlobalData;

namespace context {

const core::UString& GetSymbolString(const Context* ctx, const Symbol& sym);

const ClassSlot& GetClassSlot(const Context* ctx, Class::JSClassType type);

Symbol Intern(Context* ctx, const core::StringPiece& str);
Symbol Intern(Context* ctx, const core::UStringPiece& str);
Symbol Intern(Context* ctx, const JSString* str);
Symbol Intern(Context* ctx, uint32_t index);
Symbol Intern(Context* ctx, double number);

Symbol Lookup(Context* ctx, const core::StringPiece& str, bool* res);
Symbol Lookup(Context* ctx, const core::UStringPiece& str, bool* res);
Symbol Lookup(Context* ctx, const JSString* str, bool* res);
Symbol Lookup(Context* ctx, uint32_t index, bool* res);
Symbol Lookup(Context* ctx, double number, bool* res);

GlobalData* Global(Context* ctx);

JSString* EmptyString(Context* ctx);
JSString* LookupSingleString(Context* ctx, uint16_t ch);

JSFunction* throw_type_error(Context* ctx);

bool IsStrict(const Context* ctx);

JSVal* StackGain(Context* ctx, std::size_t size);
void StackRelease(Context* ctx, std::size_t size);

void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

} } }  // namespace iv::lv5::context
#endif  // _IV_LV5_CONTEXT_UTILS_H_
