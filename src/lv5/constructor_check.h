#ifndef _IV_LV5_CONSTRUCTOR_CHECK_H_
#define _IV_LV5_CONSTRUCTOR_CHECK_H_

// msg must be string literal
#define IV_LV5_CONSTRUCTOR_CHECK(msg, args, error)\
  do {\
    if (args.IsConstructorCalled()) {\
      error->Report(Error::Type,\
                    msg " is not a constructor");\
      return JSUndefined;\
    }\
  } while (0)

#endif  // _IV_LV5_CONSTRUCTOR_CHECK_H_
