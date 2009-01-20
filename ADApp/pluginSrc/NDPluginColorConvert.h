#ifndef NDPluginColorConvert_H
#define NDPluginColorConvert_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

typedef enum
{
    NDPluginColorConvertColorModeOut
               /* (NDColorMode_t r/w) Output color mode */
        = NDPluginDriverLastParam,
    NDPluginColorConvertLastParam
} NDPluginStdArraysParam_t;

#define NUM_COLOR_CONVERT_PARAMS (sizeof(NDPluginColorConvertParamString)/sizeof(NDPluginColorConvertParamString[0]))


class NDPluginColorConvert : public NDPluginDriver {
public:
    NDPluginColorConvert(const char *portName, int queueSize, int blockingCallbacks, 
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);

    /* These methods are just for this class */
    template <typename epicsType> void convertColor(NDArray *pArray);
};

    
#endif
