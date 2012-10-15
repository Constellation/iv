#ifndef IV_LV5_CONTEXT_UTILS_H_
#define IV_LV5_CONTEXT_UTILS_H_
#include <iv/ustring.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/space.h>
#include <iv/i18n.h>
#include <iv/lv5/class.h>
#include <iv/lv5/symbol.h>
namespace iv {
namespace lv5 {

class JSVal;
class JSString;
class Context;
class JSRegExpImpl;
class JSFunction;
class GlobalData;
class Map;

namespace context {

const ClassSlot& GetClassSlot(const Context* ctx, Class::JSClassType type);

GlobalData* Global(Context* ctx);

JSString* EmptyString(Context* ctx);
JSString* LookupSingleString(Context* ctx, uint16_t ch);

Map* GetEmptyObjectMap(Context* ctx);
Map* GetFunctionMap(Context* ctx);
Map* GetArrayMap(Context* ctx);
Map* GetStringMap(Context* ctx);
Map* GetBooleanMap(Context* ctx);
Map* GetNumberMap(Context* ctx);
Map* GetDateMap(Context* ctx);
Map* GetRegExpMap(Context* ctx);
Map* GetErrorMap(Context* ctx);

core::Space* GetRegExpAllocator(Context* ctx);

JSFunction* throw_type_error(Context* ctx);

core::i18n::I18N* I18N(Context* ctx);

bool IsStrict(const Context* ctx);

void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

JSVal* StackGain(Context* ctx, std::size_t size);
void StackRestore(Context* ctx, JSVal* pointer);

} } }  // namespace iv::lv5::context
#endif  // IV_LV5_CONTEXT_UTILS_H_
