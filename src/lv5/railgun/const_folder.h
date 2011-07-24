#ifndef _IV_LV5_RAILGUN_CONST_FOLDER_H_
#define _IV_LV5_RAILGUN_CONST_FOLDER_H_
#include "noncopyable.h"
#include "lv5/specialized_ast.h"
namespace iv {
namespace lv5 {
namespace railgun {

class ConstFolder
  : public core::Noncopyable<ConstFolder>,
    public ExpressionVisitor {
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CONST_FOLDER_H_
