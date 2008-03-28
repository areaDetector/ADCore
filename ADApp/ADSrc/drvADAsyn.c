/*
 * drvADAsyn.c
 * 
 * Asyn driver for area detectors
 *
 * Original Author: Mark Rivers
 * Current Author: Mark Rivers
 *
 * Created March 6, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iocsh.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsMessageQueue.h>
#include <errlog.h>
#include <cantProceed.h>

#include <asynDriver.h>
#include <asynInt32.h>
#include <asynInt8Array.h>
#include <asynInt16Array.h>
#include <asynInt32Array.h>
#include <asynFloat32Array.h>
#include <asynFloat64.h>
#include <asynFloat64Array.h>
#include <asynOctet.h>
#include <asynDrvUser.h>

#include <drvSup.h>
#include <registryDriverSupport.h>

#include "ADInterface.h"
#include "ADUtils.h"

#define RATE_TIME 2.0  /* Time between computing frame and image rates */

typedef enum
{
    ADCmdUpdateTime           /* (float64, r/w) Minimum time between image updates */
     = MAX_DRIVER_COMMANDS+1, /*  These commands need to avoid conflict with driver parameters */
    ADCmdPostImages,          /* (int32,   r/w) Post images (1=Yes, 0=No) */
    ADCmdImageCounter,        /* (int32,   r/w) Image counter.  Increments by 1 when image posted */
    ADCmdImageRate,           /* (float64, r/o) Image rate.  Rate at which images are being posted */
    ADCmdFrameCounter,        /* (int32,   r/w) Frame counter.  Increments by 1 when image callback */
    ADCmdFrameRate,           /* (float64, r/o) Frame rate.  Rate at which images are being received by driver */
    ADCmdImageData            /* (void*,   r/w) Image data waveform */
} ADCommand_t;

typedef struct {
    ADCommand_t command;
    char *commandString;
} ADCommandStruct;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADCommandStruct ADCommands[] = {
    {ADManufacturer,   "MANUFACTURER"},  
    {ADModel,          "MODEL"       },  
    {ADTemperature,    "TEMPERATURE" }, 
    {ADGain,           "GAIN"        },

    {ADBinX,           "BIN_X"       },
    {ADBinY,           "BIN_Y"       },

    {ADMinX,           "MIN_X"       },
    {ADMinY,           "MIN_Y"       },
    {ADSizeX,          "SIZE_X"      },
    {ADSizeY,          "SIZE_Y"      },
    {ADMaxSizeX,       "MAX_SIZE_X"  },
    {ADMaxSizeY,       "MAX_SIZE_Y"  },

    {ADImageSizeX,     "IMAGE_SIZE_X"},
    {ADImageSizeY,     "IMAGE_SIZE_Y"},
    {ADImageSize,      "IMAGE_SIZE"  },
    {ADDataType,       "DATA_TYPE"   },
    {ADFrameMode,      "FRAME_MODE"  },
    {ADNumExposures,   "NEXPOSURES"  },
    {ADNumFrames,      "NFRAMES"     },
    {ADAcquireTime,    "ACQ_TIME"    },
    {ADAcquirePeriod,  "ACQ_PERIOD"  },
    {ADConnect,        "CONNECT"     },
    {ADStatus,         "STATUS"      },
    {ADTriggerMode,    "TRIGGER_MODE"},
    {ADShutter,        "SHUTTER"     },
    {ADAcquire,        "ACQUIRE"     },

    {ADFilePath,       "FILE_PATH"     },
    {ADFileName,       "FILE_NAME"     },
    {ADFileNumber,     "FILE_NUMBER"   },
    {ADFileTemplate,   "FILE_TEMPLATE" },
    {ADAutoIncrement,  "AUTO_INCREMENT"},
    {ADFullFileName,   "FULL_FILE_NAME"},
    {ADFileFormat,     "FILE_FORMAT"   },
    {ADAutoSave,       "AUTO_SAVE"     },
    {ADWriteFile,      "WRITE_FILE"    },
    {ADReadFile,       "READ_FILE"     },

    /* These commands are for the EPICS asyn layer */
    {ADCmdUpdateTime,  "IMAGE_UPDATE_TIME" },
    {ADCmdPostImages,  "POST_IMAGES"  },
    {ADCmdImageCounter,"IMAGE_COUNTER"},
    {ADCmdImageRate,   "IMAGE_RATE"   },
    {ADCmdFrameCounter,"FRAME_COUNTER"},
    {ADCmdFrameRate,   "FRAME_RATE"   },
    {ADCmdImageData,   "IMAGE_DATA"   }
};

typedef struct drvADPvt {
    char *portName;
    ADDrvSet_t *drvset;
    DETECTOR_HDL  pDetector;
    
    /* Housekeeping */
    epicsMutexId lock;
    int rebooting;
    
    /* Detector data information */
    size_t imageSize;
    void *imageBuffer;
    
    /* Image data posting */
    double minImageUpdateTime;
    epicsTimeStamp lastRateTime;
    epicsTimeStamp lastImagePostTime;
    int imageCounter;
    int imageRateCounter;
    int frameCounter;
    int frameRateCounter;
    int postImages;
    
    /* Asyn interfaces */
    asynInterface common;
    asynInterface int32;
    void *int32InterruptPvt;
    asynInterface float64;
    void *float64InterruptPvt;
    asynInterface int8Array;
    void *int8ArrayInterruptPvt;
    asynInterface int16Array;
    void *int16ArrayInterruptPvt;
    asynInterface int32Array;
    void *int32ArrayInterruptPvt;
    asynInterface float32Array;
    void *float32ArrayInterruptPvt;
    asynInterface float64Array;
    void *float64ArrayInterruptPvt;
    asynInterface octet;
    void *octetInterruptPvt;
    asynInterface drvUser;
    asynUser *pasynUser;
} drvADPvt;

/* These functions are used by the interfaces */
static asynStatus readInt32         (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value);
static asynStatus writeInt32        (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 value);
static asynStatus getBounds         (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *low, epicsInt32 *high);
static asynStatus readInt8Array     (void *drvPvt, asynUser *pasynUser,
                                     epicsInt8 *value, size_t nelements, size_t *nIn);
static asynStatus writeInt8Array    (void *drvPvt, asynUser *pasynUser,
                                     epicsInt8 *value, size_t nelements);
static asynStatus readInt16Array    (void *drvPvt, asynUser *pasynUser,
                                     epicsInt16 *value, size_t nelements, size_t *nIn);
static asynStatus writeInt16Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsInt16 *value, size_t nelements);
static asynStatus readInt32Array    (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value, size_t nelements, size_t *nIn);
static asynStatus writeInt32Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value, size_t nelements);
static asynStatus readFloat32Array    (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat32 *value, size_t nelements, size_t *nIn);
static asynStatus writeFloat32Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat32 *value, size_t nelements);
static asynStatus readFloat64Array    (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat64 *value, size_t nelements, size_t *nIn);
static asynStatus writeFloat64Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat64 *value, size_t nelements);
static asynStatus readFloat64       (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat64 *value);
static asynStatus writeFloat64      (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat64 value);
static asynStatus readOctet         (void *drvPvt, asynUser *pasynUser,
                                     char *value, size_t maxChars, size_t *nActual,
                                     int *eomReason);
static asynStatus writeOctet        (void *drvPvt, asynUser *pasynUser,
                                     const char *value, size_t nChars, size_t *nActual);
static asynStatus drvUserCreate     (void *drvPvt, asynUser *pasynUser,
                                     const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
static asynStatus drvUserGetType    (void *drvPvt, asynUser *pasynUser,
                                     const char **pptypeName, size_t *psize);
static asynStatus drvUserDestroy    (void *drvPvt, asynUser *pasynUser);

static void report                  (void *drvPvt, FILE *fp, int details);
static asynStatus connect           (void *drvPvt, asynUser *pasynUser);
static asynStatus disconnect        (void *drvPvt, asynUser *pasynUser);


static asynCommon drvADCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 drvADInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 drvADFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet drvADOctet = {
    writeOctet,
    NULL,
    readOctet,
};

static asynInt8Array drvADInt8Array = {
    writeInt8Array,
    readInt8Array,
};

static asynInt16Array drvADInt16Array = {
    writeInt16Array,
    readInt16Array,
};

static asynInt32Array drvADInt32Array = {
    writeInt32Array,
    readInt32Array,
};

static asynFloat32Array drvADFloat32Array = {
    writeFloat32Array,
    readFloat32Array,
};

static asynFloat64Array drvADFloat64Array = {
    writeFloat64Array,
    readFloat64Array,
};

static asynDrvUser drvADDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Local functions, not in any interface */

static int allocateImageBuffer(drvADPvt *pPvt, size_t imageSize)
{
    if (imageSize > pPvt->imageSize) {
        pPvt->imageSize = imageSize;
        free(pPvt->imageBuffer);
        pPvt->imageBuffer = malloc(pPvt->imageSize*sizeof(char));
        if (!pPvt->imageBuffer) return(asynError);
    }
    return(asynSuccess);
}

static int getImageDimensions(drvADPvt *pPvt, asynUser *pasynUser,
                              int *imageSize, int *nx, int *ny, int *dataType)
{
    int status;
    
    status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADImageSize, imageSize);
    if (status != AREA_DETECTOR_OK) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::getImageDimensions error reading imageSize=%d", status);
        return(asynError);
    }
    status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADImageSizeX, nx);
    if (status != AREA_DETECTOR_OK) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::getImageDimensions error reading nx=%d", status);
        return(asynError);
    }
    status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADImageSizeY, ny);
    if (status != AREA_DETECTOR_OK) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::getImageDimensions error reading ny=%d", status);
        return(asynError);
    }
    status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADDataType, dataType);
    if (status != AREA_DETECTOR_OK) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::getImageDimensions error reading dataType=%d", status);
        return(asynError);
    }
    return(asynSuccess);
}

static int logFunc(void *userParam,
                   const ADLogMask_t logMask,
                   const char *pFormat, ...)
{
    va_list     pvar;
    asynUser    *pasynUser = (asynUser *)userParam;

    va_start(pvar, pFormat);
    switch(logMask) {
    case ADTraceError:
        pasynTrace->vprint(pasynUser, ASYN_TRACE_ERROR, pFormat, pvar);
        break;
    case ADTraceIODevice:
        pasynTrace->vprint(pasynUser, ASYN_TRACEIO_DEVICE, pFormat, pvar);
        break;
    case ADTraceIOFilter:
        pasynTrace->vprint(pasynUser, ASYN_TRACEIO_FILTER, pFormat, pvar);
        break;
    case ADTraceIODriver:
        pasynTrace->vprint(pasynUser, ASYN_TRACEIO_DRIVER, pFormat, pvar);
        break;
    case ADTraceFlow:
        pasynTrace->vprint(pasynUser, ASYN_TRACE_FLOW, pFormat, pvar);
        break;
    default:
        asynPrint(pasynUser, ASYN_TRACE_ERROR, "drvADAsyn:logFunc unknown logMask %d\n", logMask);
    }
    va_end (pvar);
    return(AREA_DETECTOR_OK);
}


/* Callback routines from driver.  These call the asyn callbacks. */

static void int32Callback(void *drvPvt, int command, int value)
{
    drvADPvt *pPvt = drvPvt;
    unsigned int reason;
    ELLLIST *pclientList;
    interruptNode *pnode;

    /* Pass int32 interrupts */
    pasynManager->interruptStart(pPvt->int32InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt32Interrupt *pInterrupt = pnode->drvPvt;
        reason = pInterrupt->pasynUser->reason;
        if (command == reason) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pPvt->int32InterruptPvt);
}

static void float64Callback(void *drvPvt, int command, double value)
{
    drvADPvt *pPvt = drvPvt;
    unsigned int reason;
    ELLLIST *pclientList;
    interruptNode *pnode;

    /* Pass float64 interrupts */
    pasynManager->interruptStart(pPvt->float64InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynFloat64Interrupt *pInterrupt = pnode->drvPvt;
        reason = pInterrupt->pasynUser->reason;
        if (command == reason) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pPvt->float64InterruptPvt);
}

static void stringCallback(void *drvPvt, int command, char *value)
{
    drvADPvt *pPvt = drvPvt;
    unsigned int reason;
    ELLLIST *pclientList;
    interruptNode *pnode;

    /* Pass octet interrupts */
    pasynManager->interruptStart(pPvt->octetInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynOctetInterrupt *pInterrupt = pnode->drvPvt;
        reason = pInterrupt->pasynUser->reason;
        if (command == reason) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value, strlen(value), ASYN_EOM_END);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pPvt->octetInterruptPvt);
}

static void imageDataCallback(void *drvPvt, void *value,  
                              ADDataType_t dataType, int nx, int ny)
{
    drvADPvt *pPvt = drvPvt;
    ELLLIST *pclientList;
    unsigned int reason;
    int nPixels = nx*ny;
    int nxt, nyt;
    interruptNode *pnode;
    epicsTimeStamp tCheck;
    double deltaTime;
    double rate;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;

    epicsTimeGetCurrent(&tCheck);
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastImagePostTime);


    /* This macro saves a lot of code, since there are 5 types of array interrupts that we support */
    #define ARRAY_INTERRUPT_CALLBACK(INTERRUPT_PVT, INTERRUPT_TYPE, INITIALIZED, \
                                     SIGNED_TYPE, UNSIGNED_TYPE, EPICS_TYPE) { \
        EPICS_TYPE *pData=NULL;\
        pasynManager->interruptStart(INTERRUPT_PVT, &pclientList); \
        pnode = (interruptNode *)ellFirst(pclientList); \
        while (pnode) { \
            INTERRUPT_TYPE *pInterrupt = pnode->drvPvt; \
            reason = pInterrupt->pasynUser->reason; \
            if (reason == ADCmdImageData) { \
                if (!pPvt->postImages) break;  /* We are not being asked to post image data */ \
                if (deltaTime < pPvt->minImageUpdateTime) break; \
                memcpy(&pPvt->lastImagePostTime, &tCheck, sizeof(epicsTimeStamp)); \
                pPvt->imageCounter++; \
                pPvt->imageRateCounter++; \
            } \
            if (!INITIALIZED) { \
                INITIALIZED = 1; \
                /* If the data type is compatible with this type just set the pointer */ \
                if ((dataType == SIGNED_TYPE) || (dataType == UNSIGNED_TYPE)) { \
                    pData = (EPICS_TYPE *)value; \
                } else { \
                    /* We need to convert the data */ \
                    allocateImageBuffer(pPvt, nPixels*sizeof(EPICS_TYPE)); \
                    ADUtils->convertImage(value, dataType, \
                                          nx, ny, \
                                          pPvt->imageBuffer, SIGNED_TYPE, \
                                          1, 1, 0, 0, \
                                          nx, ny, &nxt, &nyt); \
                    pData = (EPICS_TYPE *)pPvt->imageBuffer; \
                } \
            } \
            pInterrupt->callback(pInterrupt->userPvt, \
                                 pInterrupt->pasynUser, \
                                 pData, nPixels); \
            pnode = (interruptNode *)ellNext(&pnode->node); \
        } \
        pasynManager->interruptEnd(INTERRUPT_PVT); \
    }

    /* Pass interrupts for int8Array data*/
    ARRAY_INTERRUPT_CALLBACK(pPvt->int8ArrayInterruptPvt, asynInt8ArrayInterrupt,
                             int8Initialized, ADInt8, ADUInt8, epicsInt8);
    
    /* Pass interrupts for int16Array data*/
    ARRAY_INTERRUPT_CALLBACK(pPvt->int16ArrayInterruptPvt, asynInt16ArrayInterrupt,
                             int16Initialized, ADInt16, ADUInt16, epicsInt16);
    
    /* Pass interrupts for int32Array data*/
    ARRAY_INTERRUPT_CALLBACK(pPvt->int32ArrayInterruptPvt, asynInt32ArrayInterrupt,
                             int32Initialized, ADInt32, ADUInt32, epicsInt32);
    
    /* Pass interrupts for float32Array data*/
    ARRAY_INTERRUPT_CALLBACK(pPvt->float32ArrayInterruptPvt, asynFloat32ArrayInterrupt,
                             float32Initialized, ADFloat32, ADFloat32, epicsFloat32);
    
    /* Pass interrupts for float64Array data*/
    ARRAY_INTERRUPT_CALLBACK(pPvt->float64ArrayInterruptPvt, asynFloat64ArrayInterrupt,
                             float64Initialized, ADFloat64, ADFloat64, epicsFloat64);
    

    /* Update the frame counter.  This should be done after the image data callbacks, since
     * some clients may be monitoring this PV to see when new data are available. */
    pPvt->frameCounter++;
    pPvt->frameRateCounter++;
    int32Callback(pPvt, ADCmdFrameCounter, pPvt->frameCounter);
    int32Callback(pPvt, ADCmdImageCounter, pPvt->imageCounter);

    /* See if it is time to compute the rates */
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastRateTime);
    if (deltaTime > RATE_TIME) {
        /* First do the frame rate */
        rate = pPvt->frameRateCounter / deltaTime;
        float64Callback(pPvt, ADCmdFrameRate, rate);
        pPvt->frameRateCounter = 0;
        /* Now do the image rate */
        rate = pPvt->imageRateCounter / deltaTime;
        float64Callback(pPvt, ADCmdImageRate, rate);
        pPvt->imageRateCounter = 0;
        memcpy(&pPvt->lastRateTime, &tCheck, sizeof(epicsTimeStamp));
    }
       
}


/* asynInt32 interface methods */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    switch(command) {
    case ADCmdPostImages:
        *value = pPvt->postImages;
        break;
    case ADCmdImageCounter:
        *value = pPvt->imageCounter;
        break;
    case ADCmdFrameCounter:
        *value = pPvt->frameCounter;
        break;
    default:
        status = (*pPvt->drvset->getInteger)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADAsyn::readInt32, reason=%d, value=%d\n", 
              command, *value);
    return(status);
}

static asynStatus writeInt32(void *drvPvt, asynUser *pasynUser, 
                             epicsInt32 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    switch(command) {
    case ADCmdPostImages:
        pPvt->postImages = value;
        break;
     case ADCmdImageCounter:
        pPvt->imageCounter = value;
        int32Callback(pPvt, ADCmdImageCounter, pPvt->imageCounter);
        break;
     case ADCmdFrameCounter:
        pPvt->frameCounter = value;
        int32Callback(pPvt, ADCmdFrameCounter, pPvt->frameCounter);
        break;
   default:
        status = (*pPvt->drvset->setInteger)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
          "drvADAsyn::writeInt32, reason=%d, value=%d\n",
          command, value);
    return(status);
}

static asynStatus getBounds(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    /* This is only needed for the asynInt32 interface when the device uses raw units.
       Our interface is using engineering units. */
    *low = 0;
    *high = 65535;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADAsyn::getBounds,low=%d, high=%d\n", *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    switch(command) {
    case ADCmdUpdateTime:
        *value = pPvt->minImageUpdateTime;
        break;
    default:
        (*pPvt->drvset->getDouble)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADAsyn::readFloat64, reason=%d, value=%f\n", 
              command, *value);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser, 
                               epicsFloat64 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    switch(command) {
    case ADCmdUpdateTime:
        pPvt->minImageUpdateTime = value;
        break;
    default:
        status = (*pPvt->drvset->setDouble)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADAsyn::writeFloat64, reason=%d, value=%f\n",
              command, value);
    return(status);
}


/* asynOctet interface methods */
static asynStatus readOctet(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    switch(command) {
    default:
        status = (*pPvt->drvset->getString)(pPvt->pDetector, command, maxChars, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADAsyn::readOctet, reason=%d, value=%s\n",
              command, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    if (nChars > 0) {
        switch(command) {
        default:
            status = (*pPvt->drvset->setString)(pPvt->pDetector, command, value);
            break;
        }
    }
    *nActual = nChars;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADAsyn::writeOctet, reason=%d, value=%s\n",
              command, value);
    return(status);
}


/* The following macros save a lot of code, since we have 5 array types to support */
#define DEFINE_READ_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nelements, size_t *nIn) \
{ \
    drvADPvt *pPvt = (drvADPvt *)drvPvt; \
    ADCommand_t command = pasynUser->reason; \
    asynStatus status; \
    int imageSize, nx, ny, nPixels; \
    int nxt, nyt; \
    int dataType; \
    \
    switch(command) { \
    case ADCmdImageData: \
        status = getImageDimensions(pPvt, pasynUser, &imageSize, &nx, &ny, &dataType); \
        if (status != asynSuccess) { \
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, \
                         "drvADAsyn::READ_ARRAY error reading image dimensions\n"); \
            return(asynError); \
        }\
        nPixels = nx * ny; \
        /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */ \
        status = allocateImageBuffer(pPvt, imageSize); \
        if (status != asynSuccess) { \
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, \
                         "drvADAsyn::READ_ARRAY error allocating memory=%d", status); \
            return(asynError); \
        } \
        status = (*pPvt->drvset->getImage)(pPvt->pDetector, imageSize, pPvt->imageBuffer); \
        if (status != AREA_DETECTOR_OK) { \
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, \
                         "drvADAsyn::READ_ARRAY error reading imageBuffer=%d", status); \
            return(asynError); \
        } \
        /* Convert data from its actual data type.  */ \
        status = ADUtils->convertImage(pPvt->imageBuffer, dataType, nx, ny, \
                                       value, AD_TYPE, \
                                       1, 1, 0, 0, \
                                       nx, ny, &nxt, &nyt); \
        break; \
    default: \
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, \
                     "drvADAsyn::READ_ARRAT unknown command %d", command); \
        return(asynError); \
        break; \
    } \
    return(asynSuccess); \
}

#define DEFINE_WRITE_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nelements) \
{ \
    /* Note yet implemented */ \
    asynPrint(pasynUser, ASYN_TRACE_ERROR, \
              "drvADAsyn::WRITE_ARRAY not yet implemented\n"); \
    return(asynError); \
}

/* asynInt8Array interface methods */
DEFINE_READ_ARRAY(readInt8Array, epicsInt8, ADInt8)
DEFINE_WRITE_ARRAY(writeInt8Array, epicsInt8, ADInt8)
    
/* asynInt16Array interface methods */
DEFINE_READ_ARRAY(readInt16Array, epicsInt16, ADInt16)
DEFINE_WRITE_ARRAY(writeInt16Array, epicsInt16, ADInt16)
    
/* asynInt32Array interface methods */
DEFINE_READ_ARRAY(readInt32Array, epicsInt32, ADInt32)
DEFINE_WRITE_ARRAY(writeInt32Array, epicsInt32, ADInt32)
    
/* asynFloat32Array interface methods */
DEFINE_READ_ARRAY(readFloat32Array, epicsFloat32, ADFloat32)
DEFINE_WRITE_ARRAY(writeFloat32Array, epicsFloat32, ADFloat32)
    
/* asynFloat64Array interface methods */
DEFINE_READ_ARRAY(readFloat64Array, epicsFloat64, ADFloat64)
DEFINE_WRITE_ARRAY(writeFloat64Array, epicsFloat64, ADFloat64)
    

static void rebootCallback(void *drvPvt)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    /* Anything special we have to do on reboot */
    pPvt->rebooting = 1;
}


/* asynDrvUser routines */
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                const char *drvInfo, 
                                const char **pptypeName, size_t *psize)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int i, status;
    int ncommands = sizeof(ADCommands)/sizeof(ADCommands[0]);
    int command=-1;

    for (i=0; i < ncommands; i++) {
        if (epicsStrCaseCmp(drvInfo, ADCommands[i].commandString) == 0) {
            command = ADCommands[i].command;
            break;
        }
    }
    /* If the command was not one of the standard ones in this driver's table, then see if
     * it is a driver-specific command */
    if (command < 0) {
        status = (*pPvt->drvset->findParam)(pPvt->pDetector, drvInfo, &command);
        if (status) command = -1;
    }
 
    if (command >= 0) {
        pasynUser->reason = command;
        if (pptypeName) {
            *pptypeName = epicsStrDup(drvInfo);
        }
        if (psize) {
            *psize = sizeof(command);
        }
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
                  "drvADAsyn::drvUserCreate, command=%s, command=%d\n", drvInfo, command);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::drvUserCreate, unknown command=%s", drvInfo);
        return(asynError);
    }
}
    
static asynStatus drvUserGetType(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{
    /* This is not currently supported, because we can't get the strings for driver-specific commands */

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "drvADAsyn::drvUserGetType entered");

    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "drvADAsyn::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
          drvPvt, pasynUser);

    return(asynSuccess);
}

/* asynCommon routines */

static void report(void *drvPvt, FILE *fp, int details)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    interruptNode *pnode;
    ELLLIST *pclientList;

    fprintf(fp, "Port: %s\n", pPvt->portName);
    if (details >= 1) {
        /* Report int32 interrupts */
        pasynManager->interruptStart(pPvt->int32InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32Interrupt *pint32Interrupt = pnode->drvPvt;
            fprintf(fp, "    int32 callback client address=%p, addr=%d, reason=%d\n",
                    pint32Interrupt->callback, pint32Interrupt->addr, 
                    pint32Interrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pPvt->int32InterruptPvt);

        /* Report float64 interrupts */
        pasynManager->interruptStart(pPvt->float64InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynFloat64Interrupt *pfloat64Interrupt = pnode->drvPvt;
            fprintf(fp, "    float64 callback client address=%p, addr=%d, reason=%d\n",
                    pfloat64Interrupt->callback, pfloat64Interrupt->addr, 
                    pfloat64Interrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pPvt->float64InterruptPvt);

        /* Report asynInt32Array interrupts */
        pasynManager->interruptStart(pPvt->int32ArrayInterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32ArrayInterrupt *pint32ArrayInterrupt = pnode->drvPvt;
            fprintf(fp, "    int32Array callback client address=%p, addr=%d, reason=%d\n",
                    pint32ArrayInterrupt->callback, pint32ArrayInterrupt->addr, 
                    pint32ArrayInterrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pPvt->int32ArrayInterruptPvt);

    }
}

static asynStatus connect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionConnect(pasynUser);

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
          "drvADAsyn::connect, pasynUser=%p\n", pasynUser);
    return(asynSuccess);
}


static asynStatus disconnect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionDisconnect(pasynUser);
    return(asynSuccess);
}


/* Configuration routine, called from startup script either directly (on vxWorks) or from shell */

int drvAsynADConfigure(const char *portName, const char *driverName, int detector)
{
    drvADPvt *pPvt;
    asynStatus status;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), "drvAsynADConfigure");
    pPvt->portName = epicsStrDup(portName);
    pPvt->drvset = (ADDrvSet_t *) registryDriverSupportFind( driverName );
    if (pPvt->drvset == NULL) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't find driver: %s\n", driverName);
    return -1;
    }

    /* Link with higher level routines */
    pPvt->common.interfaceType = asynCommonType;
    pPvt->common.pinterface  = (void *)&drvADCommon;
    pPvt->common.drvPvt = pPvt;
    pPvt->int32.interfaceType = asynInt32Type;
    pPvt->int32.pinterface  = (void *)&drvADInt32;
    pPvt->int32.drvPvt = pPvt;
    pPvt->float64.interfaceType = asynFloat64Type;
    pPvt->float64.pinterface  = (void *)&drvADFloat64;
    pPvt->float64.drvPvt = pPvt;
    pPvt->int8Array.interfaceType = asynInt8ArrayType;
    pPvt->int8Array.pinterface  = (void *)&drvADInt8Array;
    pPvt->int8Array.drvPvt = pPvt;
    pPvt->int16Array.interfaceType = asynInt16ArrayType;
    pPvt->int16Array.pinterface  = (void *)&drvADInt16Array;
    pPvt->int16Array.drvPvt = pPvt;
    pPvt->int32Array.interfaceType = asynInt32ArrayType;
    pPvt->int32Array.pinterface  = (void *)&drvADInt32Array;
    pPvt->int32Array.drvPvt = pPvt;
    pPvt->float32Array.interfaceType = asynFloat32ArrayType;
    pPvt->float32Array.pinterface  = (void *)&drvADFloat32Array;
    pPvt->float32Array.drvPvt = pPvt;
    pPvt->float64Array.interfaceType = asynFloat64ArrayType;
    pPvt->float64Array.pinterface  = (void *)&drvADFloat64Array;
    pPvt->float64Array.drvPvt = pPvt;
    pPvt->octet.interfaceType = asynOctetType;
    pPvt->octet.pinterface  = (void *)&drvADOctet;
    pPvt->octet.drvPvt = pPvt;
    pPvt->drvUser.interfaceType = asynDrvUserType;
    pPvt->drvUser.pinterface  = (void *)&drvADDrvUser;
    pPvt->drvUser.drvPvt = pPvt;

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register port\n");
        return -1;
    }
    status = pasynManager->registerInterface(portName,&pPvt->common);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register common.\n");
        return -1;
    }

    status = pasynInt32Base->initialize(pPvt->portName,&pPvt->int32);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register int32\n");
        return -1;
    }
    pasynManager->registerInterruptSource(portName, &pPvt->int32,
                                          &pPvt->int32InterruptPvt);

    status = pasynFloat64Base->initialize(pPvt->portName,&pPvt->float64);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register float64\n");
        return -1;
    }
    pasynManager->registerInterruptSource(portName, &pPvt->float64,
                                          &pPvt->float64InterruptPvt);

    status = pasynInt8ArrayBase->initialize(pPvt->portName,&pPvt->int8Array);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register int8Array\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->int8Array,
                                          &pPvt->int8ArrayInterruptPvt);

    status = pasynInt16ArrayBase->initialize(pPvt->portName,&pPvt->int16Array);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register int16Array\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->int16Array,
                                          &pPvt->int16ArrayInterruptPvt);

    status = pasynInt32ArrayBase->initialize(pPvt->portName,&pPvt->int32Array);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register int32Array\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->int32Array,
                                          &pPvt->int32ArrayInterruptPvt);

    status = pasynFloat32ArrayBase->initialize(pPvt->portName,&pPvt->float32Array);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register float32Array\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->float32Array,
                                          &pPvt->float32ArrayInterruptPvt);

    status = pasynFloat64ArrayBase->initialize(pPvt->portName,&pPvt->float64Array);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register float64Array\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->float64Array,
                                          &pPvt->float64ArrayInterruptPvt);

    status = pasynOctetBase->initialize(pPvt->portName,&pPvt->octet,0,0,0);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register octet\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->octet,
                                          &pPvt->octetInterruptPvt);

    status = pasynManager->registerInterface(pPvt->portName,&pPvt->drvUser);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure ERROR: Can't register drvUser\n");
        return -1;
    }
    /* Create asynUser for debugging */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    /* Connect to device */
    status = pasynManager->connectDevice(pPvt->pasynUser, portName, 0);
    if (status != asynSuccess) {
        errlogPrintf("drvAsynADConfigure, connectDevice failed\n");
        return -1;
    }

    pPvt->lock = epicsMutexCreate();

    pPvt->pDetector = (*pPvt->drvset->open)(detector, "");
    if (!pPvt->pDetector) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "drvAsynADConfigure: Failed to open detector\n");
        return -1;
    }

    /* Setup callbacks */
    (*pPvt->drvset->setInt32Callback)    (pPvt->pDetector, int32Callback,     (void *)pPvt);
    (*pPvt->drvset->setFloat64Callback)  (pPvt->pDetector, float64Callback,   (void *)pPvt);
    (*pPvt->drvset->setStringCallback)   (pPvt->pDetector, stringCallback,    (void *)pPvt);
    (*pPvt->drvset->setImageDataCallback)(pPvt->pDetector, imageDataCallback, (void *)pPvt);

    /* All other parameters are initialised to zero at allocation */
    (*pPvt->drvset->setLog)(pPvt->pDetector, logFunc, pPvt->pasynUser );

    return 0;
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "driverName",iocshArgString};
static const iocshArg initArg2 = { "Detector number",iocshArgInt};
static const iocshArg * const initArgs[3] = {&initArg0,
                                             &initArg1,
                                             &initArg2};
static const iocshFuncDef initFuncDef = {"drvADAsynConfigure",3,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvAsynADConfigure(args[0].sval, args[1].sval, args[2].ival);
}

void ADRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(ADRegister);
