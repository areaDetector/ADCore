#ifndef NDPluginOverlay_H
#define NDPluginOverlay_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

typedef enum {
    NDOverlayCross,
    NDOverlayRectangle
} NDOverlayShape_t;

typedef enum {
    NDOverlaySet,
    NDOverlayXOR
} NDOverlayDrawMode_t;

/** Structure defining an overlay */
typedef struct NDOverlay {
    size_t PositionX;
    size_t PositionY;
    size_t SizeX;
    size_t SizeY;
    NDOverlayShape_t shape;
    NDOverlayDrawMode_t drawMode;
    int red;
    int green;
    int blue;
} NDOverlay_t;


#define NDPluginOverlayMaxSizeXString           "MAX_SIZE_X"            /* (asynInt32,   r/o) Maximum size of overlay in X dimension */
#define NDPluginOverlayMaxSizeYString           "MAX_SIZE_Y"            /* (asynInt32,   r/o) Maximum size of overlay in Y dimension */
#define NDPluginOverlayNameString               "NAME"                  /* (asynOctet,   r/w) Name of this overlay */
#define NDPluginOverlayUseString                "USE"                   /* (asynInt32,   r/w) Use this overlay? */
#define NDPluginOverlayPositionXString          "OVERLAY_POSITION_X"    /* (asynInt32,   r/o) X positoin of overlay */
#define NDPluginOverlayPositionYString          "OVERLAY_POSITION_Y"    /* (asynInt32,   r/w) X position of overlay */
#define NDPluginOverlaySizeXString              "OVERLAY_SIZE_X"        /* (asynInt32,   r/o) X size of overlay */
#define NDPluginOverlaySizeYString              "OVERLAY_SIZE_Y"        /* (asynInt32,   r/w) X size of overlay */
#define NDPluginOverlayShapeString              "OVERLAY_SHAPE"         /* (asynInt32,   r/w) Shape of overlay */
#define NDPluginOverlayDrawModeString           "OVERLAY_DRAW_MODE"     /* (asynInt32,   r/w) Drawing mode for overlay */
#define NDPluginOverlayRedString                "OVERLAY_RED"           /* (asynInt32,   r/w) Red value for overlay */
#define NDPluginOverlayGreenString              "OVERLAY_GREEN"         /* (asynInt32,   r/w) Green value for overlay */
#define NDPluginOverlayBlueString               "OVERLAY_BLUE"          /* (asynInt32,   r/w) Blue value for overlay */

/** Overlay graphics on top of an image.  Useful for highlighting ROIs and displaying cursors */
class NDPluginOverlay : public NDPluginDriver {
public:
    NDPluginOverlay(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxOverlays, 
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    template <typename epicsType> void doOverlayT(NDArray *pArray, NDOverlay_t *pOverlay);
    int doOverlay(NDArray *pArray, NDOverlay_t *pOverlay);
    template <typename epicsType> void setPixel(epicsType *pValue, NDOverlay_t *pOverlay);

protected:
    int NDPluginOverlayMaxSizeX;
    #define FIRST_NDPLUGIN_OVERLAY_PARAM NDPluginOverlayMaxSizeX
    int NDPluginOverlayMaxSizeY;
    int NDPluginOverlayName;
    int NDPluginOverlayUse;
    int NDPluginOverlayPositionX;
    int NDPluginOverlayPositionY;
    int NDPluginOverlaySizeX;
    int NDPluginOverlaySizeY;
    int NDPluginOverlayShape;
    int NDPluginOverlayDrawMode;
    int NDPluginOverlayRed;
    int NDPluginOverlayGreen;
    int NDPluginOverlayBlue;
    #define LAST_NDPLUGIN_OVERLAY_PARAM NDPluginOverlayBlue
                                
private:
    int maxOverlays;
    NDArrayInfo arrayInfo;
    NDOverlay_t *pOverlays;    /* Array of NDOverlay structures */
    NDOverlay_t *pOverlay;
};
#define NUM_NDPLUGIN_OVERLAY_PARAMS ((int)(&LAST_NDPLUGIN_OVERLAY_PARAM - &FIRST_NDPLUGIN_OVERLAY_PARAM + 1))
    
#endif
