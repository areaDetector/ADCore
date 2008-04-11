#ifndef AD_PARAM_LIB_H
#define AD_PARAM_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#define PARAM_OK (0)
#define PARAM_ERROR (-1)

typedef int paramIndex;
typedef struct paramList * PARAMS;

typedef struct
{
  PARAMS (*create)           ( paramIndex startVal, paramIndex nvals, 
                               asynStandardInterfaces *pasynInterfaces );
  void (*destroy)            ( PARAMS params );
  int  (*setInteger)         ( PARAMS params, paramIndex index, int value );
  int  (*setDouble)          ( PARAMS params, paramIndex index, double value );
  int  (*setString)          ( PARAMS params, paramIndex index, const char *value );
  int  (*getInteger)         ( PARAMS params, paramIndex index, int * value );
  int  (*getDouble)          ( PARAMS params, paramIndex index, double * value );
  int  (*getString)          ( PARAMS params, paramIndex index, int maxChars, char *value  );
  int  (*callCallbacks)      ( PARAMS params );
  void (*dump)               ( PARAMS params );
} paramSupport;

extern paramSupport * ADParam;

#ifdef __cplusplus
}
#endif
#endif
