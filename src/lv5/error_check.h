#ifndef _IV_LV5_ERROR_CHECK_H_
#define _IV_LV5_ERROR_CHECK_H_

#define IV_LV5_ERROR_GUARD_VOID(error)\
  if (*error) {\
    return;\
  }

#define IV_LV5_ERROR_GUARD_WITH(error, val)\
  if (*error) {\
    return val;\
  }

#define IV_LV5_ERROR_GUARD(error)\
  IV_LV5_ERROR_GUARD_WITH(error, JSEmpty)

#define IV_LV5_ERROR_VOID(error)\
  error);\
  IV_LV5_ERROR_GUARD_VOID(error)\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define IV_LV5_ERROR_WITH(error, val)\
  error);\
  IV_LV5_ERROR_GUARD_WITH(error, val)\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define IV_LV5_ERROR(error)\
  IV_LV5_ERROR_WITH(error, JSEmpty)

#endif  // _IV_LV5_ERROR_CHECK_H_
