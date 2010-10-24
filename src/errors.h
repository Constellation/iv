#ifndef _IV_ERRORS_H_
#define _IV_ERRORS_H_
namespace iv {
namespace core {

class Errors {
 public:
  enum Type {
    kILLEGAL = 0,
    kLEFTHANDSIDE = 0,
  };
};

} }  // namespace iv::core
#endif  // _IV_ERRORS_H_
