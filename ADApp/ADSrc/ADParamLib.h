#ifndef AD_PARAM_LIB_H
#define AD_PARAM_LIB_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PARAM_OK (0)
#define PARAM_ERROR (-1)

typedef int paramIndex;
typedef struct paramList * PARAMS;

typedef struct
{
  PARAMS (*create)                  ( paramIndex startVal, paramIndex nvals, 
                                      asynStandardInterfaces *pasynInterfaces );
  void (*destroy)                   ( PARAMS params );
  asynStatus  (*setInteger)         ( PARAMS params, paramIndex index, int value );
  asynStatus  (*setDouble)          ( PARAMS params, paramIndex index, double value );
  asynStatus  (*setString)          ( PARAMS params, paramIndex index, const char *value );
  asynStatus  (*getInteger)         ( PARAMS params, paramIndex index, int * value );
  asynStatus  (*getDouble)          ( PARAMS params, paramIndex index, double * value );
  asynStatus  (*getString)          ( PARAMS params, paramIndex index, int maxChars, char *value  );
  asynStatus  (*callCallbacks)      ( PARAMS params );
  asynStatus  (*callCallbacksAddr)  ( PARAMS params, int addr );
  void (*dump)                      ( PARAMS params );
} paramSupport;

extern paramSupport *ADParam;

#ifdef __cplusplus
}
#endif
#endif
