#ifndef AD_PARAM_LIB_H
#define AD_PARAM_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <epicsTypes.h>

#define PARAM_OK (0)
#define PARAM_ERROR (-1)

typedef  int paramIndex;
typedef struct paramList * PARAMS;
typedef void (*paramIntCallback)   ( void *, int, int ); 
typedef void (*paramDoubleCallback)( void *, int, double ); 
typedef void (*paramStringCallback)( void *, int, char * ); 

typedef struct
{
  PARAMS (*create)           ( paramIndex startVal, paramIndex nvals );
  void (*destroy)            ( PARAMS params );
  int  (*setInteger)         ( PARAMS params, paramIndex index, int value );
  int  (*setDouble)          ( PARAMS params, paramIndex index, double value );
  int  (*setString)          ( PARAMS params, paramIndex index, char *value );
  void (*callCallbacks)      ( PARAMS params );
  int  (*getInteger)         ( PARAMS params, paramIndex index, int * value );
  int  (*getDouble)          ( PARAMS params, paramIndex index, double * value );
  int  (*getString)          ( PARAMS params, paramIndex index, int maxChars, char *value  );
  int  (*setIntCallback)     ( PARAMS params, paramIntCallback callback, void * param );
  int  (*setDoubleCallback)  ( PARAMS params, paramDoubleCallback callback, void * param );
  int  (*setStringCallback)  ( PARAMS params, paramStringCallback callback, void * param );
  void (*dump)               ( PARAMS params );
} paramSupport;

extern paramSupport * ADParam;

#ifdef __cplusplus
}
#endif
#endif
