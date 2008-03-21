/*
 * drvADImageAsyn.c
 * 
 * Asyn driver for image server on area detectors
 *
 * Original Author: Mark Rivers
 * Current Author: Mark Rivers
 *
 * Created March 12, 2008
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
#include <asynInt32Array.h>
#include <asynFloat64.h>
#include <asynDrvUser.h>

#include "areaDetectorInterface.h"

typedef enum
{
    ADCmdUpdateTime,
    ADCmdPostImages,
    ADCmdImageData
} ADCommand_t;

typedef struct {
    ADCommand_t command;
    char *commandString;
} ADImageCommandStruct;

static ADCommandStruct ADCommands[] = {
    {ADCmdUpdateTime, "UPDATE_TIME"},   /* (float64, r/w) Minimum time between image updates */
    {ADCmdPostImages, "POST_IMAGES"},   /* (int32,   r/w) Post images (1=Yes, 0=No) */
    {ADCmdPostImages, "IMAGE_COUNTER"}, /* (int32,   r/w) Image counter.  Increments by 1 when image posted */
    {ADCmdImageData,  "IMAGE_DATA"  }   /* (void,    r/w) Image data waveform */
};

typedef struct drvADImagePvt {
    char *portName;
    char *ADPortName;
    /* Housekeeping */
    epicsMutexId lock;
    /* Asyn interfaces that we access on ADPort server */
    asynInterface ADInt32;
    void *ADInt32Pvt;
    asynInterface ADIFloat64;
    void *ADFloat64Pvt;
    asynInterface ADInt32Array;
    void *ADInt32ArrayPvt;
   
    /* Asyn interfaces that we export */
    asynInterface common;
    asynInterface int32;
    void *int32InterruptPvt;
    asynInterface float64;
    void *float64InterruptPvt;
    asynInterface int32Array;
    void *int32ArrayInterruptPvt;
    asynInterface drvUser;
    asynUser *pasynUser;
} drvADImagePvt;

/* These functions are used by the interfaces */
static asynStatus readInt32         (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value);
static asynStatus writeInt32        (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 value);
static asynStatus getBounds         (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *low, epicsInt32 *high);
static asynStatus readInt32Array    (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value, size_t nelements, size_t *nIn);
static asynStatus writeInt32Array   (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value, size_t nelements);
static asynStatus readFloat64       (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat64 *value);
static asynStatus writeFloat64      (void *drvPvt, asynUser *pasynUser,
                                     epicsFloat64 value);
static asynStatus drvUserCreate     (void *drvPvt, asynUser *pasynUser,
                                     const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
static asynStatus drvUserGetType    (void *drvPvt, asynUser *pasynUser,
                                     const char **pptypeName, size_t *psize);
static asynStatus drvUserDestroy    (void *drvPvt, asynUser *pasynUser);

static void report                  (void *drvPvt, FILE *fp, int details);
static asynStatus connect           (void *drvPvt, asynUser *pasynUser);
static asynStatus disconnect        (void *drvPvt, asynUser *pasynUser);


/* These are private functions, not used in any interfaces */
static void int32Callback    (void *drvPvt, ADParam_t command, int value);
static void float64Callback  (void *drvPvt, ADParam_t command, double value);
static void imageDataCallback(void *drvPvt, void *value, 
                              ADDataType_t dataType, size_t nBytes, int nx, int ny);
static int allocateImageBuffer(drvADImagePvt *pPvt, size_t imageSize);
static int convertToInt32     (drvADImagePvt *pPvt, ADDataType_t dataType, 
                               int nPixels, void *input, epicsInt32 *output);
 
static int logFunc     (void *userParam,
                        const ADLogMask_t logMask,
                        const char *pFormat, ...);

static asynCommon drvADImageCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 drvADImageInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 drvADImageFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet drvADImageOctet = {
    writeOctet,
    NULL,
    readOctet,
};

static asynInt32Array drvADImageInt32Array = {
    writeInt32Array,
    readInt32Array,
};

static asynDrvUser drvADImageDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


int drvADImageConfigure(const char *portName, const char *ADPortName)
{
    drvADImagePvt *pPvt;
    asynStatus status;
    asynInterface *pasynInterface;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), "drvADImageConfigure");
    pPvt->portName = epicsStrDup(portName);

    /* Link with higher level routines */
    pPvt->common.interfaceType = asynCommonType;
    pPvt->common.pinterface  = (void *)&drvADImageCommon;
    pPvt->common.drvPvt = pPvt;
    pPvt->int32.interfaceType = asynInt32Type;
    pPvt->int32.pinterface  = (void *)&drvADImageInt32;
    pPvt->int32.drvPvt = pPvt;
    pPvt->float64.interfaceType = asynFloat64Type;
    pPvt->float64.pinterface  = (void *)&drvADImageFloat64;
    pPvt->float64.drvPvt = pPvt;
    pPvt->int32Array.interfaceType = asynInt32ArrayType;
    pPvt->int32Array.pinterface  = (void *)&drvADImageInt32Array;
    pPvt->int32Array.drvPvt = pPvt;
    pPvt->octet.interfaceType = asynOctetType;
    pPvt->octet.pinterface  = (void *)&drvADImageOctet;
    pPvt->octet.drvPvt = pPvt;
    pPvt->drvUser.interfaceType = asynDrvUserType;
    pPvt->drvUser.pinterface  = (void *)&drvADImageDrvUser;
    pPvt->drvUser.drvPvt = pPvt;

    /* Create asynUsers to communicate with ADPort driver */
    pPvt->pInt32AsynUser = pasynManager->createAsynUser(0, 0);
    pPvt->pFloat64AsynUser = pasynManager->createAsynUser(0, 0);
    pPvt->pInt32ArrayAsynUser = pasynManager->createAsynUser(0, 0);

    /* Connect to ADPort driver  */
    status = pasynManager->connectDevice(pPvt->pInt32DAsynUser, 
                                         ADPortName, 0);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure: connectDevice failed for Int32\n");
        return -1;
    }

    /* Get the asynUInt32DigitalCallback interface */
    pasynInterface = pasynManager->findInterface(pPvt->puint32DAsynUser, 
                                               asynUInt32DigitalType, 1);
    if (!pasynInterface) {
        errlogPrintf("initQuadEM, find asynUInt32Digital interface failed\n");
        return -1;
    }
    pPvt->uint32Digital = (asynUInt32Digital *)pasynInterface->pinterface;
    pPvt->uint32DigitalPvt = pasynInterface->drvPvt;

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register port\n");
        return -1;
    }
    status = pasynManager->registerInterface(portName,&pPvt->common);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register common.\n");
        return -1;
    }

    status = pasynInt32Base->initialize(pPvt->portName,&pPvt->int32);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register int32\n");
        return -1;
    }
    pasynManager->registerInterruptSource(portName, &pPvt->int32,
                                          &pPvt->int32InterruptPvt);

    status = pasynFloat64Base->initialize(pPvt->portName,&pPvt->float64);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register float64\n");
        return -1;
    }
    pasynManager->registerInterruptSource(portName, &pPvt->float64,
                                          &pPvt->float64InterruptPvt);

    status = pasynInt32ArrayBase->initialize(pPvt->portName,&pPvt->int32Array);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register int32Array\n");
        return -1;
    }

    pasynManager->registerInterruptSource(portName, &pPvt->int32Array,
                                          &pPvt->int32ArrayInterruptPvt);

    status = pasynOctetBase->initialize(pPvt->portName,&pPvt->octet,0,0,0);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register octet\n");
        return -1;
    }

    status = pasynManager->registerInterface(pPvt->portName,&pPvt->drvUser);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure ERROR: Can't register drvUser\n");
        return -1;
    }
    /* Create asynUser for debugging */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    /* Connect to device */
    status = pasynManager->connectDevice(pPvt->pasynUser, portName, 0);
    if (status != asynSuccess) {
        errlogPrintf("drvADImageConfigure, connectDevice failed\n");
        return -1;
    }

    pPvt->lock = epicsMutexCreate();

    pPvt->pDetector = (*pPvt->drvset->open)(detector, "");
    if (!pPvt->pDetector) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "drvADImageConfigure: Failed to open detector\n");
        return -1;
    }

    /* Setup callbacks */
    (*pPvt->drvset->setInt32Callback)    (pPvt->pDetector, int32Callback,     (void *)pPvt);
    (*pPvt->drvset->setFloat64Callback)  (pPvt->pDetector, float64Callback,   (void *)pPvt);
    (*pPvt->drvset->setImageDataCallback)(pPvt->pDetector, imageDataCallback, (void *)pPvt);

    /* All other parameters are initialised to zero at allocation */
    (*pPvt->drvset->setLog)(pPvt->pDetector, logFunc, pPvt->pasynUser );

    return 0;
}


/* asynInt32 interface methods */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    int addr;
    ADCommand_t command = pasynUser->reason;

    pasynManager->getAddr(pasynUser, &addr);

    switch(command) {
    default:
        (*pPvt->drvset->getInteger)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADImage::readInt32, reason=%d, value=%d\n", 
              command, *value);
    return(asynSuccess);
}

static asynStatus writeInt32(void *drvPvt, asynUser *pasynUser, 
                             epicsInt32 value)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    int addr;
    ADCommand_t command = pasynUser->reason;
    asynStatus status;

    pasynManager->getAddr(pasynUser, &addr);

    switch(command) {
    default:
        status = (*pPvt->drvset->setInteger)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
          "drvADImage::writeInt32, reason=%d, value=%d\n",
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
              "drvADImage::getBounds,low=%d, high=%d\n", *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynSuccess;

    switch(command) {
    case ADCmdUpdateTime:
        *value = pPvt->updateTime;
        break;
    default:
        status = asynError;
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADImage::readFloat64, reason=%d, value=%f\n", 
              command, *value);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser, 
                               epicsFloat64 value)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynError;

    switch(command) {
    case ADCmdUpdateTime:
        pPvt->updateTime = value;
        break;
    default:
        status = asynError;
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADImage::writeFloat64, reason=%d, value=%f\n",
          command, value);
    return(status);
}


/* asynOctet interface methods */
static asynStatus readOctet(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    int addr;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynError;

    pasynManager->getAddr(pasynUser, &addr);

    switch(command) {
    default:
        status = (*pPvt->drvset->getString)(pPvt->pDetector, command, maxChars, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADImage::readOctet, reason=%d, value=%s\n",
              command, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    int addr;
    ADCommand_t command = pasynUser->reason;
    asynStatus status = asynError;

    pasynManager->getAddr(pasynUser, &addr);

    switch(command) {
    default:
        status = (*pPvt->drvset->setString)(pPvt->pDetector, command, value);
        break;
    }
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "drvADImage::writeOctet, reason=%d, value=%s\n",
              command, value);
    return(status);
}



/* asynInt32Array interface methods */
static asynStatus readInt32Array(void *drvPvt, asynUser *pasynUser,
                                 epicsInt32 *value, size_t nelements, size_t *nIn)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    ADCommand_t command = pasynUser->reason;
    asynStatus status;
    int imageSize, nx, ny, nPixels;
    int dataType;
    
    switch(command) {
    case ADCmdImageData:
        status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADImageSize, &imageSize);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADImage::readInt32Array error reading imageSize=%d", status);
            return(asynError);
        }
        status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADSizeX, &nx);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADImage::readInt32Array error reading nx=%d", status);
            return(asynError);
        }
        status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADSizeY, &ny);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADImage::readInt32Array error reading ny=%d", status);
            return(asynError);
        }
        nPixels = nx * ny;
        status = (*pPvt->drvset->getInteger)(pPvt->pDetector, ADDataType, &dataType);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADImage::readInt32Array error reading dataType=%d", status);
            return(asynError);
        }
        /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */
        status = allocateImageBuffer(pPvt, imageSize);
        if (status != asynSuccess) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADImage::readInt32Array error allocating memory=%d", status);
            return(asynError);
        }
        status = (*pPvt->drvset->getImage)(pPvt->pDetector, imageSize, pPvt->imageBuffer);
        if (status != AREA_DETECTOR_OK) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "drvADImage::readInt32Array error reading imageBuffer=%d", status);
            return(asynError);
        }
        /* Convert data from its actual data type to epicsInt32.  */
        status = convertToInt32(pPvt, dataType, nPixels, pPvt->imageBuffer, value);
        break;
    default:
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADImage::readInt32Array unknown command %d", command);
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
              "drvADImage::writeInt32Array not yet implemented\n");
    return(asynError);
   
}

static int allocateImageBuffer(drvADImagePvt *pPvt, size_t imageSize)
{
    if (imageSize > pPvt->imageSize) {
        pPvt->imageSize = imageSize;
        free(pPvt->imageBuffer);
        pPvt->imageBuffer = malloc(pPvt->imageSize*sizeof(char));
        if (!pPvt->imageBuffer) return(asynError);
    }
    return(asynSuccess);
}

static int convertToInt32(drvADImagePvt *pPvt, ADDataType_t dataType, 
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
        "drvADImage::convertToInt32 unknown dataType %d", dataType);
        return(asynError);
        break;
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
        asynPrint(pasynUser, ASYN_TRACE_ERROR, "drvADImage:logFunc unknown logMask %d\n", logMask);
    }
    va_end (pvar);
    return(AREA_DETECTOR_OK);
}

static void int32Callback(void *drvPvt, ADCommand_t command, int value)
{
    drvADImagePvt *pPvt = drvPvt;
    unsigned int reason;
    ELLLIST *pclientList;
    interruptNode *pnode;

    /* Pass int32 interrupts */
    pasynManager->interruptStart(pPvt->int32InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt32Interrupt *pint32Interrupt = pnode->drvPvt;
        reason = pint32Interrupt->pasynUser->reason;
        if (command == reason) {
            pint32Interrupt->callback(pint32Interrupt->userPvt, 
                                      pint32Interrupt->pasynUser,
                                      value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pPvt->int32InterruptPvt);
}

static void float64Callback(void *drvPvt, ADCommand_t command, double value)
{
    drvADImagePvt *pPvt = drvPvt;
    unsigned int reason;
    ELLLIST *pclientList;
    interruptNode *pnode;

    /* Pass float64 interrupts */
    pasynManager->interruptStart(pPvt->float64InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynFloat64Interrupt *pfloat64Interrupt = pnode->drvPvt;
        reason = pfloat64Interrupt->pasynUser->reason;
        if (command == reason) {
            pfloat64Interrupt->callback(pfloat64Interrupt->userPvt, 
                                        pfloat64Interrupt->pasynUser,
                                        value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pPvt->float64InterruptPvt);
}

static void imageDataCallback(void *drvPvt, void *value,  
                              ADDataType_t dataType, size_t nBytes, int nx, int ny)
{
    drvADImagePvt *pPvt = drvPvt;
    ELLLIST *pclientList;
    unsigned int reason;
    int nPixels = nx*ny;
    interruptNode *pnode;
    epicsInt32 *pInt32Data=NULL;
    int int32Initialized=0;

    /* Pass interrupts for int32Array data*/
    pasynManager->interruptStart(pPvt->int32ArrayInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt32ArrayInterrupt *pint32ArrayInterrupt = pnode->drvPvt;
        reason = pint32ArrayInterrupt->pasynUser->reason;
        if (reason == ADCmdImageData) {
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
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pPvt->int32ArrayInterruptPvt);
}

static void rebootCallback(void *drvPvt)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
    /* Anything special we have to do on reboot */
    pPvt->rebooting = 1;
}

/* asynDrvUser routines */
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                const char *drvInfo, 
                                const char **pptypeName, size_t *psize)
{
    int i;
    char *pstring;
    int ncommands = sizeof(ADCommands)/sizeof(ADCommands[0]);

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "drvADImage::drvUserCreate, drvInfo=%s, pptypeName=%p, psize=%p, pasynUser=%p\n", 
              drvInfo, pptypeName, psize, pasynUser);

    for (i=0; i < ncommands; i++) {
        pstring = ADCommands[i].commandString;
        if (epicsStrCaseCmp(drvInfo, pstring) == 0) {
            break;
        }
    }
    if (i < ncommands) {
        pasynUser->reason = ADCommands[i].command;
        if (pptypeName) {
            *pptypeName = epicsStrDup(pstring);
        }
        if (psize) {
            *psize = sizeof(ADCommands[i].command);
        }
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
                  "drvADImage::drvUserCreate, command=%s\n", pstring);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "drvADImage::drvUserCreate, unknown command=%s", drvInfo);
        return(asynError);
    }
}
    
static asynStatus drvUserGetType(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{
    ADCommand_t command = pasynUser->reason;

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "drvADImage::drvUserGetType entered");

    *pptypeName = NULL;
    *psize = 0;
    if (pptypeName)
        *pptypeName = epicsStrDup(ADCommands[command].commandString);
    if (psize) *psize = sizeof(command);
    return(asynSuccess);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "drvADImage::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
          drvPvt, pasynUser);

    return(asynSuccess);
}

/* asynCommon routines */

/* Report  parameters */
static void report(void *drvPvt, FILE *fp, int details)
{
    drvADImagePvt *pPvt = (drvADImagePvt *)drvPvt;
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

    }
}

/* Connect */
static asynStatus connect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionConnect(pasynUser);

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
          "drvADImage::connect, pasynUser=%p\n", pasynUser);
    return(asynSuccess);
}

/* Disconnect */
static asynStatus disconnect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionDisconnect(pasynUser);
    return(asynSuccess);
}

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "Area detector portName",iocshArgString};
static const iocshArg * const initArgs[2] = {&initArg0,
                                             &initArg1}
static const iocshFuncDef initFuncDef = {"drvADImageConfigure",2,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvADImageConfigure(args[0].sval, args[1].sval, args[2].ival);
}

void Register(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(ADImageRegister);
