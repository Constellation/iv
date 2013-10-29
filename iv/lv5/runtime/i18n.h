#ifndef IV_LV5_RUNTIME_I18N_H_
#define IV_LV5_RUNTIME_I18N_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

#ifdef IV_ENABLE_I18N
JSVal CollatorConstructor(const Arguments& args, Error* e);

JSVal CollatorCompareGetter(const Arguments& args, Error* e);

JSVal CollatorResolvedOptions(const Arguments& args, Error* e);

JSVal CollatorSupportedLocalesOf(const Arguments& args, Error* e);
#endif  // IV_ENABLE_I18N

JSVal NumberFormatConstructor(const Arguments& args, Error* e);

JSVal NumberFormatFormatGetter(const Arguments& args, Error* e);

JSVal NumberFormatFormat(const Arguments& args, Error* e);

JSVal NumberFormatResolvedOptions(const Arguments& args, Error* e);

JSVal NumberFormatSupportedLocalesOf(const Arguments& args, Error* e);

#ifdef IV_ENABLE_I18N

JSVal DateTimeFormatConstructor(const Arguments& args, Error* e);

JSVal DateTimeFormatFormat(const Arguments& args, Error* e);

JSVal DateTimeFormatResolvedOptions(const Arguments& args, Error* e);

JSVal DateTimeFormatSupportedLocalesOf(const Arguments& args, Error* e);
#endif  // IV_ENABLE_I18N

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_I18N_H_
