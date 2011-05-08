#ifndef _IV_LV5_ERROR_CHECK_H_
#define _IV_LV5_ERROR_CHECK_H_

#define IV_LV5_ERROR_VOID(error)\
  error);\
  if (*error) {\
    return;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define IV_LV5_ERROR_WITH(error, val)\
  error);\
  if (*error) {\
    return val;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define IV_LV5_ERROR(error)\
  IV_LV5_ERROR_WITH(error, JSUndefined)

#endif  // _IV_LV5_ERROR_CHECK_H_
