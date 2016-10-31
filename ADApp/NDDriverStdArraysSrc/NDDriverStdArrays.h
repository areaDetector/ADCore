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

protected:
    int NDSAArrayMode;                 /* 0: Overwrite, 1: Append                */
#define FIRST_NDSA_DETECTOR_PARAM NDSAArrayMode
    int NDSAPartialArrayCallbacks;     /* 0: Disable, 1: Enable                  */
    int NDSANumElements;               /* Num elements to append in Append mode. */
    int NDSACurrentPixel;              /* Append beginning at this pixel.        */
#define LAST_NDSA_DETECTOR_PARAM NDSACurrentPixel

private:
    template <typename epicsType> int writeXXXArray();
};

#define arrayModeString                "ARRAY_MODE"                  /* (asynInt32,        r/w) Overwrite or append         */
#define partialArrayCallbacksString    "PARTIAL_ARRAY_CALLBACKS"     /* (asynInt32,        r/w) Disable or Enable           */
#define numElementsString              "NUM_ELEMENTS"                /* (asynInt32,        r/w) Number of pixels to append  */
#define currentPixelString             "CURRENT_PIXEL"               /* (asynInt32,        r/w) Append beginning this pixel */

#define NUM_NDSA_DETECTOR_PARAMS ((int)(&LAST_NDSA_DETECTOR_PARAM - &FIRST_NDSA_DETECTOR_PARAM + 1))
