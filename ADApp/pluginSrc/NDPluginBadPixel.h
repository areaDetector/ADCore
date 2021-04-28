#ifndef NDPluginProcess_H
#define NDPluginProcess_H

#include <vector>

#include "NDPluginDriver.h"

typedef struct {
    int x;
    int y;
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
    std::vector<pixelCoordinate> medianCoordinates;
} badPixelDef;

/* Bad pixel file*/
#define NDPluginBadPixelFileNameString "BAD_PIXEL_FILE_NAME"    /* (asynOctet,   r/w) Name of the bad pixel file */

class NDPLUGIN_API NDPluginBadPixel : public NDPluginDriver {
public:
    NDPluginBadPixel(const char *portName, int queueSize, int blockingCallbacks,
                     const char *NDArrayPort, int NDArrayAddr,
                     int maxBuffers, size_t maxMemory,
                     int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);

protected:
    /* Background array subtraction */
    int NDPluginBadPixelFileName;
    #define FIRST_NDPLUGIN_BAD_PIXEL_PARAM NDPluginBadPixelFileName

private:
    asynStatus readBadPixelFile(const char* fileName);
    std::vector<badPixelDef> badPixelList;
};

#endif
