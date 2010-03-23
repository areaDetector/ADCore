#ifndef NDPluginOverlay_H
#define NDPluginOverlay_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

typedef enum {
    NDOverlayCross,
    NDOverlayRectangle
} NDOverlayType_t;

typedef enum {
    NDOverlaySet,
    NDOverlayXOR
} NDOverlayDrawMode_t;

/** Structure defining an overlay */
typedef struct NDOverlay {
    int XPosition;
    int YPosition;
    int XSize;
    int YSize;
    NDOverlayType_t type;
    NDOverlayDrawMode_t drawMode;
    int red;
    int green;
    int blue;
} NDOverlay_t;


#define NDPluginOverlayNameString               "NAME"                  /* (asynOctet,   r/w) Name of this overlay */
#define NDPluginOverlayUseString                "USE"                   /* (asynInt32,   r/w) Use this overlay? */
#define NDPluginOverlayXPositionString          "OVERLAY_X_POSITION"    /* (asynInt32,   r/o) X positoin of overlay */
#define NDPluginOverlayYPositionString          "OVERLAY_Y_POSITION"    /* (asynInt32,   r/w) X position of overlay */
#define NDPluginOverlayXSizeString              "OVERLAY_X_SIZE"        /* (asynInt32,   r/o) X size of overlay */
#define NDPluginOverlayYSizeString              "OVERLAY_Y_SIZE"        /* (asynInt32,   r/w) X size of overlay */
#define NDPluginOverlayTypeString               "OVERLAY_TYPE"          /* (asynInt32,   r/w) Type of overlay */
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

protected:
    int NDPluginOverlayName;
    #define FIRST_NDPLUGIN_OVERLAY_PARAM NDPluginOverlayName
    int NDPluginOverlayUse;
    int NDPluginOverlayXPosition;
    int NDPluginOverlayYPosition;
    int NDPluginOverlayXSize;
    int NDPluginOverlayYSize;
    int NDPluginOverlayType;
    int NDPluginOverlayDrawMode;
    int NDPluginOverlayRed;
    int NDPluginOverlayGreen;
    int NDPluginOverlayBlue;
    #define LAST_NDPLUGIN_OVERLAY_PARAM NDPluginOverlayBlue
                                
private:
    int maxOverlays;
    NDOverlay_t *pOverlays;    /* Array of NDOverlay structures */
};
#define NUM_NDPLUGIN_OVERLAY_PARAMS (&LAST_NDPLUGIN_OVERLAY_PARAM - &FIRST_NDPLUGIN_OVERLAY_PARAM + 1)
    
#endif
