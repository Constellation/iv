#ifndef IV_LV5_ATTRIBUTES_H_
#define IV_LV5_ATTRIBUTES_H_
namespace iv {
namespace lv5 {

class ATTR {
 public:
  enum Attribute {
    NONE = 0,
    WRITABLE = 1,
    ENUMERABLE = 2,
    CONFIGURABLE = 4,
    UNDEF_WRITABLE = 8,
    UNDEF_ENUMERABLE = 16,
    UNDEF_CONFIGURABLE = 32,
    DATA = 64,
    ACCESSOR = 128,
    EMPTY = 256,
    UNDEF_VALUE = 512,
    UNDEF_GETTER = 1024,
    UNDEF_SETTER = 2048,

    // short options
    N = NONE,
    W = WRITABLE,
    E = ENUMERABLE,
    C = CONFIGURABLE
  };

  static const int DEFAULT =
      UNDEF_WRITABLE | UNDEF_ENUMERABLE | UNDEF_CONFIGURABLE |
      UNDEF_VALUE | UNDEF_GETTER | UNDEF_SETTER;
  static const int TYPE_MASK = DATA | ACCESSOR;
  static const int DATA_ATTR_MASK = WRITABLE | ENUMERABLE | CONFIGURABLE;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_ATTRIBUTES_H_
