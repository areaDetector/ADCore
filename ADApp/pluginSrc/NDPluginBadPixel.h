#ifndef NDPluginProcess_H
#define NDPluginProcess_H

#include <vector>

#include "NDPluginDriver.h"

typedef struct {
    size_t x;
    size_t y;
} pixelCoordinate;

typedef enum {
    badPixelModeSet,
    badPixelModeReplace,
    badPixelModeMedian
} badPixelMode;

typedef struct {
    pixelCoordinate coordinate;
    badPixelMode mode;
    pixelCoordinate replaceCoordinate;
    double setValue;
    int medianSize;
} badPixel_t;

/* Bad pixel file*/
#define NDPluginBadPixelFileNameString "BAD_PIXEL_FILE_NAME"    /* (asynOctet,   r/w) Name of the bad pixel file */

class NDPLUGIN_API NDPluginBadPixel : public NDPluginDriver {
public:
    NDPluginBadPixel(const char *portName, int queueSize, int blockingCallbacks,
                     const char *NDArrayPort, int NDArrayAddr,
                     int maxBuffers, size_t maxMemory,
                     int priority, int stackSize, int maxThreads);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);
    void report(FILE *fp, int details);

protected:
    /* Background array subtraction */
    int NDPluginBadPixelFileName;
    #define FIRST_NDPLUGIN_BAD_PIXEL_PARAM NDPluginBadPixelFileName

private:
    template <typename epicsType> void fixBadPixelsT(NDArray *pArray, std::vector<badPixel_t> &badPixels, NDArrayInfo_t *pArrayInfo);
    int fixBadPixels(NDArray *pArray, std::vector<badPixel_t> &badPixels, NDArrayInfo_t *pArrayInfo);
    asynStatus readBadPixelFile(const char* fileName);
    epicsInt64 computePixelOffset(pixelCoordinate coord, NDArrayInfo_t *pArrayInfo);
    std::vector<badPixel_t> badPixelList;
};

#endif
