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
    int NDSA_ArrayMode_;                 /* 0: Overwrite, 1: Append                */
#define FIRST_NDSA_DRIVER_PARAM NDSA_ArrayMode_
    int NDSA_PartialArrayCallbacks_;     /* 0: Disable, 1: Enable                  */
    int NDSA_NumElements_;               /* Num elements to append in Append mode. */
    int NDSA_CurrentPixel_;              /* Append beginning at this pixel.        */
    int NDSA_NDimensions_;               /* Number of dimensions                   */
    int NDSA_Dimensions_;                /* Array of dimensions                    */
    int NDSA_ArrayData_;                 /* Array data                             */
#define LAST_NDSA_DRIVER_PARAM NDSA_ArrayData_

private:
    template <typename epicsType> asynStatus writeXXXArray(asynUser *pasynUser, void *pValue, size_t nElements);
    template <typename epicsType, typename NDArrayType> asynStatus copyBuffer(NDArray *pArray, void *pValue, size_t nElements);
    size_t arrayDimensions_[ND_ARRAY_MAX_DIMS];
    void *pNewData_;
};

#define NDSA_ArrayModeString                "NDSA_ARRAY_MODE"                  /* (asynInt32,        r/w) Overwrite or append         */
#define NDSA_PartialArrayCallbacksString    "NDSA_PARTIAL_ARRAY_CALLBACKS"     /* (asynInt32,        r/w) Disable or Enable           */
#define NDSA_NumElementsString              "NDSA_NUM_ELEMENTS"                /* (asynInt32,        r/w) Number of pixels to append  */
#define NDSA_CurrentPixelString             "NDSA_CURRENT_PIXEL"               /* (asynInt32,        r/w) Append beginning this pixel */
#define NDSA_NDimensionsString              "NDSA_NDIMENSIONS"                 /* (asynInt32,        r/o) Number of dimensions */
#define NDSA_DimensionsString               "NDSA_DIMENSIONS"                  /* (asynInt32,        r/o) Array dimensions */
#define NDSA_ArrayDataString                "NDSA_ARRAY_DATA"                  /* (asynXXXArray,     r/o) Array data */

#define NUM_NDSA_DRIVER_PARAMS ((int)(&LAST_NDSA_DRIVER_PARAM - &FIRST_NDSA_DRIVER_PARAM + 1))
