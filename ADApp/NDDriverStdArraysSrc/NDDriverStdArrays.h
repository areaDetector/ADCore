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
  NDSA_EveryUpdate,
  NDSA_WhenComplete
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
    int NDSA_CallbackMode_;              /* 0: Every update, 1: When complete      */
#define FIRST_NDSA_DRIVER_PARAM NDSA_CallbackMode_
    int NDSA_NumElements_;               /* Number of elements currently in array  */
    int NDSA_NextElement_;               /* Next element to write to in array  */
    int NDSA_NDimensions_;               /* Number of dimensions                   */
    int NDSA_Dimensions_;                /* Array of dimensions                    */
    int NDSA_ArrayData_;                 /* Array data                             */
#define LAST_NDSA_DRIVER_PARAM NDSA_ArrayData_

private:
    template <typename epicsType> asynStatus writeXXXArray(asynUser *pasynUser, void *pValue, size_t nElements);
    template <typename epicsType, typename NDArrayType> void copyBuffer(NDArray *pArray, void *pValue, size_t nElements);
    size_t arrayDimensions_[ND_ARRAY_MAX_DIMS];
    void *pNewData_;
    size_t maxElements_;
    NDArray *pArray_;
};

#define NDSA_CallbackModeString             "NDSA_CALLBACK_MODE"               /* (asynInt32,        r/w) Every update or when complete         */
#define NDSA_NumElementsString              "NDSA_NUM_ELEMENTS"                /* (asynInt32,        r/o) Number of elements currently in array */
#define NDSA_NextElementString              "NDSA_NEXT_ELEMENT"                /* (asynInt32,        r/w) Next element to write to in array */
#define NDSA_NDimensionsString              "NDSA_NDIMENSIONS"                 /* (asynInt32,        r/o) Number of dimensions                  */
#define NDSA_DimensionsString               "NDSA_DIMENSIONS"                  /* (asynInt32,        r/o) Array dimensions                      */
#define NDSA_ArrayDataString                "NDSA_ARRAY_DATA"                  /* (asynXXXArray,     r/o) Array data                            */

#define NUM_NDSA_DRIVER_PARAMS ((int)(&LAST_NDSA_DRIVER_PARAM - &FIRST_NDSA_DRIVER_PARAM + 1))
