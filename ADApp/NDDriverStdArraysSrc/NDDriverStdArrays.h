/* NDDriverStdArrays.h
 * 
 * This is a driver for converting standard EPICS arrays (waveform records)
 * into NDArrays.
 *
 * It allows any Channel Access client to inject NDArrays into an areaDetector IOC.
 * 
 * Author: David J. Vine
 * Berkeley National Lab
 *
 * Mark Rivers
 * University of Chicago
 * 
 * Created: September 28, 2016
 *
 */


#include "ADDriver.h"

typedef enum {
  NDSA_OnUpdate,
  NDSA_OnComplete,
  NDSA_OnCommand
} NDSA_CallbackMode_t;

class epicsShareClass NDDriverStdArrays : public ADDriver {
public:
    NDDriverStdArrays(const char *portName,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    
    virtual asynStatus writeInt8Array (asynUser *pasynUser,      epicsInt8 *value, size_t nElements);
    virtual asynStatus writeInt16Array(asynUser *pasynUser,     epicsInt16 *value, size_t nElements);
    virtual asynStatus writeInt32Array(asynUser *pasynUser,     epicsInt32 *value, size_t nElements);
    virtual asynStatus writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements);
    virtual asynStatus writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual void report (FILE *fp, int details);

protected:
    int NDSA_CallbackMode_;
#define FIRST_NDSA_DRIVER_PARAM NDSA_CallbackMode_
    int NDSA_DoCallbacks_;
    int NDSA_AppendMode_;
    int NDSA_NumElements_;
    int NDSA_NextElement_;
    int NDSA_NewArray_;
    int NDSA_ArrayComplete_;
    int NDSA_ArrayData_;
#define LAST_NDSA_DRIVER_PARAM NDSA_ArrayData_

private:
    template <typename epicsType> asynStatus writeXXXArray(asynUser *pasynUser, void *pValue, size_t nElements);
    template <typename epicsType, typename NDArrayType> void copyBuffer(size_t nextElement, void *pValue, size_t nElements);
    void doCallbacks();
    void setArrayComplete();
    size_t arrayDimensions_[ND_ARRAY_MAX_DIMS];
};

#define NDSA_CallbackModeString             "NDSA_CALLBACK_MODE"               /* (asynInt32,        r/w) Every update, when complete           */
#define NDSA_DoCallbacksString              "NDSA_DO_CALLBACKS"                /* (asynInt32,        r/w) Force callbacks                       */
#define NDSA_AppendModeString               "NDSA_APPEND_MODE"                 /* (asynInt32,        r/w) Enable or disable                     */
#define NDSA_NumElementsString              "NDSA_NUM_ELEMENTS"                /* (asynInt32,        r/o) Number of elements currently in array */
#define NDSA_NextElementString              "NDSA_NEXT_ELEMENT"                /* (asynInt32,        r/w) Next element to write to in array     */
#define NDSA_NewArrayString                 "NDSA_NEW_ARRAY"                   /* (asynInt32,        r/o) Start a new array in append mode      */
#define NDSA_ArrayCompleteString            "NDSA_ARRAY_COMPLETE"              /* (asynInt32,        r/o) Array is complete in append mode      */
#define NDSA_ArrayDataString                "NDSA_ARRAY_DATA"                  /* (asynXXXArray,     r/o) Array data                            */

#define NUM_NDSA_DRIVER_PARAMS ((int)(&LAST_NDSA_DRIVER_PARAM - &FIRST_NDSA_DRIVER_PARAM + 1))
