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
#include <asynFloat64.h>
#include <asynOctet.h>
#include <asynDrvUser.h>

#include <drvSup.h>
#include <registryDriverSupport.h>

#include "ADInterface.h"

typedef enum
{
    ADCmdUpdateTime           /* (float64, r/w) Minimum time between image updates */
     = MAX_DRIVER_COMMANDS+1, /*  These commands need to avoid conflict with driver parameters */
    ADCmdPostImages,          /* (int32,   r/w) Post images (1=Yes, 0=No) */
    ADCmdImageCounter,        /* (int32,   r/w) Image counter.  Increments by 1 when image posted */
    ADCmdFrameCounter,        /* (int32,   r/w) Frame counter.  Increments by 1 when image callback */
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
    {ADSaveFile,       "SAVE_FILE"     },

    /* These commands are for the EPICS asyn layer */
    {ADCmdUpdateTime,  "IMAGE_UPDATE_TIME" },
    {ADCmdPostImages,  "POST_IMAGES" },
    {ADCmdImageCounter,"IMAGE_COUNTER"},
    {ADCmdFrameCounter,"FRAME_COUNTER"},
    {ADCmdImageData,   "IMAGE_DATA"  }
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
    epicsTimeStamp lastImagePostTime;
    int imageCounter;
    int frameCounter;
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

static asynDrvUser drvADDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Local functions, not in any interface */

static int convertToInt8(drvADPvt *pPvt, ADDataType_t dataType, 
                         int nPixels, void *input, epicsInt8 *output)
{
    /* Converts data from native data type to epicsInt8 */
    int i;

    switch(dataType) {
    case ADInt8: {
        memcpy(output, input, nPixels/sizeof(epicsInt8));       
        break; }
   case ADUInt8: {
        memcpy(output, input, nPixels/sizeof(epicsUInt8));       
        break; }
    case ADInt16: {
        epicsInt16 *buff = (epicsInt16 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt8) buff[i];       
        break; }
    case ADUInt16: {
        epicsUInt16 *buff = (epicsUInt16 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt8) buff[i];       
        break; }
    case ADInt32: {
        epicsInt32 *buff = (epicsInt32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt8) buff[i];       
        break; }
    case ADUInt32: {
        epicsUInt32 *buff = (epicsUInt32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt8) buff[i];       
        break; }
    case ADFloat32: {
        epicsFloat32 *buff = (epicsFloat32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt8) buff[i];       
        break; }
    case ADFloat64: {
        epicsFloat64 *buff = (epicsFloat64 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt8) buff[i];       
        break; }
    default:
        epicsSnprintf(pPvt->pasynUser->errorMessage, pPvt->pasynUser->errorMessageSize,
                      "drvADAsyn::convertToInt8 unknown dataType %d", dataType);
        return(asynError);
        break;
    }
    return(asynSuccess);
}


static int convertToInt16(drvADPvt *pPvt, ADDataType_t dataType, 
                          int nPixels, void *input, epicsInt16 *output)
{
    /* Converts data from native data type to epicsInt16 */
    int i;

    switch(dataType) {
    case ADInt8: {
        epicsInt8 *buff = (epicsInt8 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt16) buff[i];       
        break; }
   case ADUInt8: {
        epicsUInt8 *buff = (epicsUInt8 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt16) buff[i];       
        break; }
    case ADInt16: {
        memcpy(output, input, nPixels/sizeof(epicsInt16));       
        break; }
    case ADUInt16: {
        memcpy(output, input, nPixels/sizeof(epicsUInt16));       
        break; }
    case ADInt32: {
        epicsInt32 *buff = (epicsInt32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt16) buff[i];       
        break; }
    case ADUInt32: {
        epicsUInt32 *buff = (epicsUInt32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt16) buff[i];       
        break; }
    case ADFloat32: {
        epicsFloat32 *buff = (epicsFloat32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt16) buff[i];       
        break; }
    case ADFloat64: {
        epicsFloat64 *buff = (epicsFloat64 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt16) buff[i];       
        break; }
    default:
        epicsSnprintf(pPvt->pasynUser->errorMessage, pPvt->pasynUser->errorMessageSize,
                      "drvADAsyn::convertToInt16 unknown dataType %d", dataType);
        return(asynError);
        break;
    }
    return(asynSuccess);
}

static int convertToInt32(drvADPvt *pPvt, ADDataType_t dataType, 
                          int nPixels, void *input, epicsInt32 *output)
{
    /* Converts data from native data type to epicsInt32 */
    int i;

    switch(dataType) {
    case ADInt8: {
        epicsInt8 *buff = (epicsInt8 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt32) buff[i];
        break; }
   case ADUInt8: {
        epicsUInt8 *buff = (epicsUInt8 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt32) buff[i];
        break; }
    case ADInt16: {
        epicsInt16 *buff = (epicsInt16 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt32) buff[i];       
        break; }
    case ADUInt16: {
        epicsUInt16 *buff = (epicsUInt16 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt32) buff[i];       
        break; }
    case ADInt32: {
        memcpy(output, input, nPixels/sizeof(epicsInt32));       
        break; }
    case ADUInt32: {
        memcpy(output, input, nPixels/sizeof(epicsUInt32));       
        break; }
    case ADFloat32: {
        epicsFloat32 *buff = (epicsFloat32 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt32) buff[i];       
        break; }
    case ADFloat64: {
        epicsFloat64 *buff = (epicsFloat64 *)input;
        for (i=0; i<nPixels; i++) output[i] = (epicsInt32) buff[i];       
        break; }
    default:
        epicsSnprintf(pPvt->pasynUser->errorMessage, pPvt->pasynUser->errorMessageSize,
                      "drvADAsyn::convertToInt32 unknown dataType %d", dataType);
        return(asynError);
        break;
    }
    return(asynSuccess);
}

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
                              ADDataType_t dataType, size_t nBytes, int nx, int ny)
{
    drvADPvt *pPvt = drvPvt;
    ELLLIST *pclientList;
    unsigned int reason;
    int nPixels = nx*ny;
    interruptNode *pnode;
    epicsTimeStamp tCheck;
    double deltaTime;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    epicsInt8  *pInt8Data=NULL;
    epicsInt16 *pInt16Data=NULL;
    epicsInt32 *pInt32Data=NULL;

    epicsTimeGetCurrent(&tCheck);
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastImagePostTime);

    /* Update the frame counter */
    pPvt->frameCounter++;
    int32Callback(pPvt, ADCmdFrameCounter, pPvt->frameCounter);

    /* Pass interrupts for int8Array data*/
    pasynManager->interruptStart(pPvt->int8ArrayInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt8ArrayInterrupt *pint8ArrayInterrupt = pnode->drvPvt;
        reason = pint8ArrayInterrupt->pasynUser->reason;
        if (reason == ADCmdImageData) {
            if (!pPvt->postImages) break;  /* We are not being asked to post image data */
            if (deltaTime < pPvt->minImageUpdateTime) break;
            memcpy(&pPvt->lastImagePostTime, &tCheck, sizeof(epicsTimeStamp));
        }
        if (!int8Initialized) {
            int8Initialized = 1;
            /* If the data type is compatible with epicsInt8 just set the pointer */
            if ((dataType == ADInt8) || (dataType == ADUInt8)) {
                pInt8Data = (epicsInt8*)value;
            } else {
                /* We need to convert the data to epicsInt8 */
                allocateImageBuffer(pPvt, nPixels*sizeof(epicsInt8));
                convertToInt8(pPvt, dataType, nPixels, value, (epicsInt8 *)pPvt->imageBuffer);
                pInt8Data = (epicsInt8*)pPvt->imageBuffer;
            }
        }
        pint8ArrayInterrupt->callback(pint8ArrayInterrupt->userPvt, 
                                      pint8ArrayInterrupt->pasynUser,
                                      pInt8Data, nPixels);
        pnode = (interruptNode *)ellNext(&pnode->node);
        if (reason == ADCmdImageData) {
            /* Update the image Counter */
            pPvt->imageCounter++;
            int32Callback(pPvt, ADCmdImageCounter, pPvt->imageCounter);
        }
    }
    pasynManager->interruptEnd(pPvt->int32ArrayInterruptPvt);

    /* Pass interrupts for int16Array data*/
    pasynManager->interruptStart(pPvt->int16ArrayInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt16ArrayInterrupt *pint16ArrayInterrupt = pnode->drvPvt;
        reason = pint16ArrayInterrupt->pasynUser->reason;
        if (reason == ADCmdImageData) {
            if (!pPvt->postImages) break;  /* We are not being asked to post image data */
            if (deltaTime < pPvt->minImageUpdateTime) break;
            memcpy(&pPvt->lastImagePostTime, &tCheck, sizeof(epicsTimeStamp));
        }
        if (!int16Initialized) {
            int16Initialized = 1;
            /* If the data type is compatible with epicsInt16 just set the pointer */
            if ((dataType == ADInt16) || (dataType == ADUInt16)) {
                pInt16Data = (epicsInt16*)value;
            } else {
                /* We need to convert the data to epicsInt16 */
                allocateImageBuffer(pPvt, nPixels*sizeof(epicsInt16));
                convertToInt16(pPvt, dataType, nPixels, value, (epicsInt16 *)pPvt->imageBuffer);
                pInt16Data = (epicsInt16*)pPvt->imageBuffer;
            }
        }
        pint16ArrayInterrupt->callback(pint16ArrayInterrupt->userPvt, 
                                       pint16ArrayInterrupt->pasynUser,
                                       pInt16Data, nPixels);
        pnode = (interruptNode *)ellNext(&pnode->node);
        if (reason == ADCmdImageData) {
            /* Update the image Counter */
            pPvt->imageCounter++;
            int32Callback(pPvt, ADCmdImageCounter, pPvt->imageCounter);
        }
    }
    pasynManager->interruptEnd(pPvt->int32ArrayInterruptPvt);

    /* Pass interrupts for int32Array data*/
    pasynManager->interruptStart(pPvt->int32ArrayInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt32ArrayInterrupt *pint32ArrayInterrupt = pnode->drvPvt;
        reason = pint32ArrayInterrupt->pasynUser->reason;
        if (reason == ADCmdImageData) {
            if (!pPvt->postImages) break;  /* We are not being asked to post image data */
            if (deltaTime < pPvt->minImageUpdateTime) break;
            memcpy(&pPvt->lastImagePostTime, &tCheck, sizeof(epicsTimeStamp));
        }
        if (!int32Initialized) {
            int32Initialized = 1;
            /* If the data type is compatible with epicsInt32 just set the pointer */
            if ((dataType == ADInt32) || (dataType == ADUInt32)) {
                pInt32Data = (epicsInt32*)value;
            } else {
                /* We need to convert the data to epicsInt32 */
                allocateImageBuffer(pPvt, nPixels*sizeof(epicsInt32));
                convertToInt32(pPvt, dataType, nPixels, value, (epicsInt32 *)pPvt->imageBuffer);
                pInt32Data = (epicsInt32*)pPvt->imageBuffer;
            }
        }
        pint32ArrayInterrupt->callback(pint32ArrayInterrupt->userPvt, 
                                       pint32ArrayInterrupt->pasynUser,
                                       pInt32Data, nPixels);
        pnode = (interruptNode *)ellNext(&pnode->node);
        if (reason == ADCmdImageData) {
            /* Update the image Counter */
            pPvt->imageCounter++;
            int32Callback(pPvt, ADCmdImageCounter, pPvt->imageCounter);
        }
    }
    pasynManager->interruptEnd(pPvt->int32ArrayInterruptPvt);
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


/* asynInt8Array interface methods */
static asynStatus readInt8Array(void *drvPvt, asynUser *pasynUser,
                                epicsInt8 *value, size_t nelements, size_t *nIn)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status;
    int imageSize, nx, ny, nPixels;
    int dataType;
    
    switch(command) {
    case ADCmdImageData:
        status = getImageDimensions(pPvt, pasynUser, &imageSize, &nx, &ny, &dataType);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt8Array error reading image dimensions\n");
            return(asynError);
        }

        nPixels = nx * ny;
        /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */
        status = allocateImageBuffer(pPvt, imageSize);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt8Array error allocating memory=%d", status);
            return(asynError);
        }
        status = (*pPvt->drvset->getImage)(pPvt->pDetector, imageSize, pPvt->imageBuffer);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt8Array error reading imageBuffer=%d", status);
            return(asynError);
        }
        /* Convert data from its actual data type to epicsInt8.  */
        status = convertToInt8(pPvt, dataType, nPixels, pPvt->imageBuffer, value);
        break;
    default:
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::readInt8Array unknown command %d", command);
        return(asynError);
        break;
    }
    return(asynSuccess);
}


static asynStatus writeInt8Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsInt8 *value, size_t nelements)
{
    /* Note yet implemented */
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "drvADAsyn::writeInt8Array not yet implemented\n");
    return(asynError);
   
}


/* asynInt16Array interface methods */
static asynStatus readInt16Array(void *drvPvt, asynUser *pasynUser,
                                 epicsInt16 *value, size_t nelements, size_t *nIn)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status;
    int imageSize, nx, ny, nPixels;
    int dataType;
    
    switch(command) {
    case ADCmdImageData:
        status = getImageDimensions(pPvt, pasynUser, &imageSize, &nx, &ny, &dataType);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt16Array error reading image dimensions\n");
            return(asynError);
        }

        nPixels = nx * ny;
        /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */
        status = allocateImageBuffer(pPvt, imageSize);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt16Array error allocating memory=%d", status);
            return(asynError);
        }
        status = (*pPvt->drvset->getImage)(pPvt->pDetector, imageSize, pPvt->imageBuffer);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt16Array error reading imageBuffer=%d", status);
            return(asynError);
        }
        /* Convert data from its actual data type to epicsInt16.  */
        status = convertToInt16(pPvt, dataType, nPixels, pPvt->imageBuffer, value);
        break;
    default:
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::readInt16Array unknown command %d", command);
        return(asynError);
        break;
    }
    return(asynSuccess);
}


static asynStatus writeInt16Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsInt16 *value, size_t nelements)
{
    /* Note yet implemented */
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "drvADAsyn::writeInt16Array not yet implemented\n");
    return(asynError);
   
}


/* asynInt32Array interface methods */
static asynStatus readInt32Array(void *drvPvt, asynUser *pasynUser,
                                 epicsInt32 *value, size_t nelements, size_t *nIn)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status;
    int imageSize, nx, ny, nPixels;
    int dataType;
    
    switch(command) {
    case ADCmdImageData:
        status = getImageDimensions(pPvt, pasynUser, &imageSize, &nx, &ny, &dataType);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt32Array error reading image dimensions\n");
            return(asynError);
        }

        nPixels = nx * ny;
        /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */
        status = allocateImageBuffer(pPvt, imageSize);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt32Array error allocating memory=%d", status);
            return(asynError);
        }
        status = (*pPvt->drvset->getImage)(pPvt->pDetector, imageSize, pPvt->imageBuffer);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADAsyn::readInt32Array error reading imageBuffer=%d", status);
            return(asynError);
        }
        /* Convert data from its actual data type to epicsInt32.  */
        status = convertToInt32(pPvt, dataType, nPixels, pPvt->imageBuffer, value);
        break;
    default:
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADAsyn::readInt32Array unknown command %d", command);
        return(asynError);
        break;
    }
    return(asynSuccess);
}


static asynStatus writeInt32Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value, size_t nelements)
{
    /* Note yet implemented */
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "drvADAsyn::writeInt32Array not yet implemented\n");
    return(asynError);
   
}


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
        if (!status) command = -1;
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
