#ifndef NDPluginProcess_H
#define NDPluginProcess_H

#include <vector>

#include "NDPluginDriver.h"

// We use epicsInt64 rather than size_t in these structs because we need to do signed arithmetic on these values
typedef struct {
    epicsInt64 x;
    epicsInt64 y;
} pixelCoordinate;

typedef enum {
    badPixelModeSet,
    badPixelModeReplace,
    badPixelModeMedian
} badPixelMode;

class badPixel {
    public:
        badPixel(pixelCoordinate coord) {
            coordinate = coord;
        }
        friend bool operator<(const badPixel& lhs, const badPixel& rhs) {
            if (lhs.coordinate.y != rhs.coordinate.y) 
                return (lhs.coordinate.y < rhs.coordinate.y);
            else
                return (lhs.coordinate.x < rhs.coordinate.x);
        }
        pixelCoordinate coordinate;
        badPixelMode mode;
        pixelCoordinate replaceCoordinate;
        double setValue;
        pixelCoordinate medianCoordinate;
    private:
        badPixel(); // Default constructor is private so objects cannot be constructed without arguments
};

typedef struct {
    epicsInt64 sizeX;
    epicsInt64 sizeY;
    epicsInt64 offsetX;
    epicsInt64 offsetY;
    int binX;
    int binY;
} badPixDimInfo_t;

typedef std::set<badPixel> badPixelList_t;
/* Bad pixel file*/
#define NDPluginBadPixelFileNameString "BAD_PIXEL_FILE_NAME"    /* (asynOctet,   r/w) Name of the bad pixel file */

class NDPLUGIN_API NDPluginBadPixel : public NDPluginDriverParamSet, public NDPluginDriver {
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
    template <typename epicsType> void fixBadPixelsT(NDArray *pArray, badPixelList_t &badPixels, NDArrayInfo_t *pArrayInfo);
    int fixBadPixels(NDArray *pArray, badPixelList_t &badPixels, NDArrayInfo_t *pArrayInfo);
    asynStatus readBadPixelFile(const char* fileName);
    epicsInt64 computePixelOffset(pixelCoordinate coord, badPixDimInfo_t& dimInfo, NDArrayInfo_t *pArrayInfo);
    badPixelList_t badPixelList;
};

#endif
