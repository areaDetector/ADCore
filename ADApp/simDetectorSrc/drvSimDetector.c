#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <epicsFindSymbol.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <iocsh.h>
#include <drvSup.h>
#include <epicsExport.h>

#define DEFINE_AREA_DETECTOR_PROTOTYPES 1
#include "ADParamLib.h"
#include "ADInterface.h"

/* If we have any private driver commands they begin with ADFirstDriverCommand and should end
   with ADLastDriverParam, which is used for setting the size of the parameter library table */

typedef enum {
   SimModeRamp,
   SimModeSin
} SimMode_t;
   
typedef enum {
   SimDelay = ADFirstDriverParam,
   SimMode,
   ADLastDriverParam
} SimParam_t;

typedef struct {
    SimParam_t command;
    char *commandString;
} SimCommandStruct;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static SimCommandStruct SimCommands[] = {
    {SimDelay,   "SIM_DELAY"},  
    {SimMode,    "SIM_MODE"}  
};

ADDrvSeT_t ADSimDetector = 
  {
    18,
    ADReport,            /* Standard EPICS driver report function (optional) */
    ADInit,              /* Standard EPICS driver initialisation function (optional) */
    ADSetLog,            /* Defines an external logging function (optional) */
    ADOpen,              /* Driver open function */
    ADClose,             /* Driver close function */
    ADFindParam,         /* Parameter lookup function */
    ADSetInt32Callback,     /* Provides a callback function the driver can call when an int32 value updates */
    ADSetFloat64Callback,   /* Provides a callback function the driver can call when a float64 value updates */
    ADSetStringCallback,    /* Provides a callback function the driver can call when a string value updates */
    ADSetImageDataCallback, /* Provides a callback function the driver can call when the image data updates */
    ADGetInteger,        /* Pointer to function to get an integer value */
    ADSetInteger,        /* Pointer to function to set an integer value */
    ADGetDouble,         /* Pointer to function to get a double value */
    ADSetDouble,         /* Pointer to function to set a double value */
    ADGetString,         /* Pointer to function to get a string value */
    ADSetString,         /* Pointer to function to set a string value */
    ADGetImage,          /* Pointer to function to read image data */
    ADSetImage           /* Pointer to function to write image data */
  };

epicsExportAddress(drvet, ADSimDetector);

typedef struct ADHandle {
    /* The first set of items in this structure will be needed by all drivers */
    int camera;                        /* Index of this camera in list of controlled cameras */
    epicsMutexId ADLock;               /* A Mutex to lock access to data structures. */
    ADLogFunc logFunc;                 /* These are for error and debug logging.*/
    void *logParam;
    PARAMS params;
    ADImageDataCallbackFunc pImageDataCallback;
    void *imageDataCallbackParam;

    /* These items are specific to the Simulator driver */
    int framesRemaining;
    epicsEventId eventId;
    void *imageBuffer;
    int bufferSize;
    int imageSize;
} camera_t;

static int ADLogMsg(void * param, const ADLogMask_t logMask, const char *pFormat, ...);

#define PRINT   (pCamera->logFunc)
#define TRACE_FLOW    ADTraceFlow
#define TRACE_ERROR   ADTraceError
#define TRACE_IODRIVER  ADTraceIODriver

static int numCameras;

/* Pointer to array of controller strutures */
static camera_t *allCameras=NULL;


static int ADBytesPerPixel(int dataType, int *size)
{
    int numTypes;
    int sizes[] = AD_BYTES_PER_PIXEL;

    numTypes = sizeof(sizes)/sizeof(size_t);
    if (dataType < numTypes) {
        *size = sizes[dataType];
        return AREA_DETECTOR_OK;
    }
    return AREA_DETECTOR_ERROR;
}

static void ADComputeImage(DETECTOR_HDL pCamera)
{
    int bytesPerPixel;
    int status = AREA_DETECTOR_OK;
    int dataType;
    int maxSizeX, maxSizeY, imageSize;
    double exposureTime, gain, intensity, pixelValue;
    int simMode;
    int i;
    epicsUInt8 *pData, pxlValInt;

    PRINT(pCamera->logParam, TRACE_FLOW, "ADComputeImage: entry\n");
    ADParam->getInteger(pCamera->params, ADMaxSizeX, &maxSizeX);
    ADParam->getInteger(pCamera->params, ADMaxSizeY, &maxSizeY);
    ADParam->getInteger(pCamera->params, ADDataType, &dataType);
    ADParam->getDouble(pCamera->params, ADAcquireTime, &exposureTime);
    ADParam->getDouble(pCamera->params, ADGain, &gain);
    ADParam->getInteger(pCamera->params, SimMode, &simMode);

    /* Make sure the image buffer we have allocated is large enough */
    status = ADBytesPerPixel(dataType, &bytesPerPixel);
    imageSize = maxSizeX * maxSizeY * bytesPerPixel;
    if (imageSize > pCamera->bufferSize) {
        free(pCamera->imageBuffer);
        pCamera->imageBuffer = calloc(bytesPerPixel, maxSizeX*maxSizeY);
        pCamera->bufferSize = imageSize;
    }
    pData=(epicsUInt8 *)pCamera->imageBuffer;
    
    /* We just make a linear ramp for now */
    /* The pixel increments are gain*exposure time in ms */
    intensity = gain * exposureTime / 1000.;
    pixelValue = 0.;
    for (i=0; i<maxSizeX*maxSizeY; i++) {
       (*pData++) = (epicsUInt8)pixelValue;
       pixelValue += intensity;
    }

}

static void ADUpdateImage(DETECTOR_HDL pCamera)
{
    int i;
    int dataType;
    int sizeX, sizeY, imageSize, binX, binY, startX, startY;
    int simMode;
    epicsUInt8 *pData=(epicsUInt8 *)pCamera->imageBuffer;

    PRINT(pCamera->logParam, TRACE_FLOW, "ADUpdateImage: entry\n");
    ADParam->getInteger(pCamera->params, ADSizeX, &sizeX);
    ADParam->getInteger(pCamera->params, ADSizeY, &sizeY);
    ADParam->getInteger(pCamera->params, ADDataType, &dataType);
    ADParam->getInteger(pCamera->params, SimMode, &simMode);

    /* Update the image.  Just increment each pixel by 1 for now */
    for (i=0, pData=pCamera->imageBuffer; i<sizeX*sizeY; i++) {
        (*pData++)++;
    }
    /* Get the subimage, including binning */
}    


static void ADSimulateTask(DETECTOR_HDL pCamera)
{
    /* This thread computes new frame data */
    int status = AREA_DETECTOR_OK;
    int dataType;
    int sizeX, sizeY, imageSize;
    int acquire;
    double delay;

    /* Loop forever */
    while (1) {
    
        /* Is acquisition active? */
        ADParam->getInteger(pCamera->params, ADAcquire, &acquire);
        
        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
             PRINT(pCamera->logParam, TRACE_FLOW, "ADSimulateTask: waiting for acquire to start\n");
             status = epicsEventWait(pCamera->eventId);
        }
        
        /* We are acquiring */
        
        /* Update the image */
        ADUpdateImage(pCamera);
        
        /* Get the current parameters */
        ADParam->getInteger(pCamera->params, ADSizeX, &sizeX);
        ADParam->getInteger(pCamera->params, ADSizeY, &sizeY);
        ADParam->getInteger(pCamera->params, ADImageSize, &imageSize);
        ADParam->getInteger(pCamera->params, ADDataType, &dataType);

        /* Call the imageData callback */
        PRINT(pCamera->logParam, TRACE_FLOW, "ADSimulateTask: calling imageData callback\n");
        pCamera->pImageDataCallback(pCamera->imageDataCallbackParam, 
                                    pCamera->imageBuffer,
                                    dataType, imageSize, sizeX, sizeY);

        /* See if acquisition is done */
        if (pCamera->framesRemaining > 0) pCamera->framesRemaining--;
        if (pCamera->framesRemaining == 0) {
            ADParam->setInteger(pCamera->params, ADAcquire, 0);
            ADParam->setInteger(pCamera->params, ADStatus, ADStatusIdle);
            ADParam->callCallbacks(pCamera->params);
            PRINT(pCamera->logParam, TRACE_FLOW, "ADSimulateTask: acquisition completed\n");
        }
        
        /* Wait for the frame delay period */
        ADParam->getDouble(pCamera->params, SimDelay, &delay);
        if (delay > 0.0) epicsThreadSleep(delay);
    }
}

static void ADReport(int level)
{
    int i;
    DETECTOR_HDL pCamera;

    for(i=0; i<numCameras; i++) {
        pCamera = &allCameras[i];
        printf("Simulation detector %d\n", i);
        if (level > 0) {
            int nx, ny;
            ADParam->getInteger(pCamera->params, ADSizeX, &nx);
            ADParam->getInteger(pCamera->params, ADSizeY, &ny);
            printf("  NX, NY:            %d  %d\n", nx, ny);
        }
        if (level > 5) {
            printf("\nParameter library contents:\n");
            ADParam->dump(pCamera->params);
        }
    }   
}


static int ADInit(void)
{
    return AREA_DETECTOR_OK;
}

static int ADSetLog( DETECTOR_HDL pCamera, ADLogFunc logFunc, void * param )
{
    if (logFunc == NULL)
    {
        pCamera->logFunc=ADLogMsg;
        pCamera->logParam = NULL;
    }
    else
    {
        pCamera->logFunc=logFunc;
        pCamera->logParam = param;
    }
    return AREA_DETECTOR_OK;
}

static DETECTOR_HDL ADOpen(int card, char * param)
{
    DETECTOR_HDL pCamera;

    if (card >= numCameras) return(NULL);
    pCamera = &allCameras[card];
    return pCamera;
}

static int ADClose(DETECTOR_HDL pCamera)
{
    return AREA_DETECTOR_OK;
}

static int ADFindParam( DETECTOR_HDL pDetector, const char *paramString, int *function )
{
    int i;
    int ncommands = sizeof(SimCommands)/sizeof(SimCommands[0]);

    for (i=0; i < ncommands; i++) {
        if (epicsStrCaseCmp(paramString, SimCommands[i].commandString) == 0) {
            *function = SimCommands[i].command;
            return AREA_DETECTOR_OK;
        }
    }
    return AREA_DETECTOR_ERROR;
}

static int ADSetInt32Callback(DETECTOR_HDL pCamera, ADInt32CallbackFunc callback, void * param)
{
    int status = AREA_DETECTOR_OK;
    
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;
    
    status = ADParam->setIntCallback(pCamera->params, callback, param);
    
    return(status);
}

static int ADSetFloat64Callback(DETECTOR_HDL pCamera, ADFloat64CallbackFunc callback, void * param)
{
    int status = AREA_DETECTOR_OK;
    
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;
    
    status = ADParam->setDoubleCallback(pCamera->params, callback, param);
    
    return(status);
}

static int ADSetStringCallback(DETECTOR_HDL pCamera, ADStringCallbackFunc callback, void * param)
{
    int status = AREA_DETECTOR_OK;
    
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;
    
    status = ADParam->setStringCallback(pCamera->params, callback, param);
    
    return(status);
}

static int ADSetImageDataCallback(DETECTOR_HDL pCamera, ADImageDataCallbackFunc callback, void * param)
{
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;
    pCamera->pImageDataCallback = callback;
    pCamera->imageDataCallbackParam = param;
    return AREA_DETECTOR_OK;
}

static int ADGetInteger(DETECTOR_HDL pCamera, int function, int * value)
{
    int status = AREA_DETECTOR_OK;
    
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;
    
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getInteger(pCamera->params, function, value);
    if (status) PRINT(pCamera->logParam, TRACE_ERROR, "ADGetInteger error, status=%d function=%d, value=%d\n", 
                      status, function, *value);
    return(status);
}

static int ADSetInteger(DETECTOR_HDL pCamera, int function, int value)
{
    int status = AREA_DETECTOR_OK;

    if (pCamera == NULL) return AREA_DETECTOR_ERROR;

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setInteger(pCamera->params, function, value);

    /* For a real detector this is where the parameter is sent to the hardware */
    switch (function) {
    case ADAcquire:
        if (value) {
            /* We need to set the number of frames we expect to collect, so the frame callback function
               can know when acquisition is complete.  We need to find out what mode we are in and how
               many frames have been requested.  If we are in continuous mode then set the number of
               remaining frames to -1. */
            int frameMode, numFrames;
            status |= ADParam->getInteger(pCamera->params, ADFrameMode, &frameMode);
            status |= ADParam->getInteger(pCamera->params, ADNumFrames, &numFrames);
            switch(frameMode) {
            case ADSingleFrame:
                pCamera->framesRemaining = 1;
                break;
            case ADMultipleFrame:
                pCamera->framesRemaining = numFrames;
                break;
            case ADContinuousFrame:
                pCamera->framesRemaining = -1;
                break;
            }
            /* Compute the initial image */
            ADComputeImage(pCamera);
            /* Send an event to wake up the simulation task */
            epicsEventSignal(pCamera->eventId);
        } 
        break;
    }
    
    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pCamera->params);
    
    if (status) PRINT(pCamera->logParam, TRACE_ERROR, "ADSetInteger error, status=%d, function=%d, value=%d\n", 
                      status, function, value);
    return status;
}


static int ADGetDouble(DETECTOR_HDL pCamera, int function, double * value)
{
    int status = AREA_DETECTOR_OK;
    
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;

    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getDouble(pCamera->params, function, value);

    if (status) PRINT(pCamera->logParam, TRACE_ERROR, "ADGetDouble error, status=%d, function=%d, value=%f\n", 
                      status, function, *value);
    return(status);
}

static int ADSetDouble(DETECTOR_HDL pCamera, int function, double value)
{
    int status = AREA_DETECTOR_OK;

    if (pCamera == NULL) return AREA_DETECTOR_ERROR;

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setDouble(pCamera->params, function, value);

    /* For a real detector we need to act on specific functions here, and download them to the driver */

    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pCamera->params);
    
    if (status) PRINT(pCamera->logParam, TRACE_ERROR, "ADSetDouble error, status=%d, function=%d, value=%f\n", 
                      status, function, value);
    return status;
}

static int ADGetString(DETECTOR_HDL pCamera, int function, int maxChars, char * value)
{
    int status = AREA_DETECTOR_OK;
   
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;

    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getString(pCamera->params, function, maxChars, value);

    if (status) PRINT(pCamera->logParam, TRACE_ERROR, "ADGetString error, status=%d, function=%d, value=%s\n", 
                      status, function, value);
    return(status);
}

static int ADSetString(DETECTOR_HDL pCamera, int function, const char *value)
{
    int status = AREA_DETECTOR_OK;

    if (pCamera == NULL) return AREA_DETECTOR_ERROR;

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setString(pCamera->params, function, (char *)value);

    if (status) PRINT(pCamera->logParam, TRACE_ERROR, "ADGetString error, status=%d, function=%d, value=%s\n", 
                      status, function, value);
    return status;
}

static int ADGetImage(DETECTOR_HDL pCamera, int maxBytes, void *buffer)
{
    int nCopy;
    
    if (pCamera == NULL) return AREA_DETECTOR_ERROR;
    nCopy = pCamera->imageSize;
    if (nCopy > maxBytes) nCopy = maxBytes;
    memcpy(buffer, pCamera->imageBuffer, nCopy);
    
    return AREA_DETECTOR_OK;
}

static int ADSetImage(DETECTOR_HDL pCamera, int maxBytes, void *buffer)
{

    if (pCamera == NULL) return AREA_DETECTOR_ERROR;

    /* The simDetector does not allow downloading image data */    
    return AREA_DETECTOR_ERROR;
}


static int ADLogMsg(void * param, const ADLogMask_t mask, const char *pFormat, ...)
{

    va_list     pvar;
    int         nchar;

    va_start(pvar, pFormat);
    nchar = vfprintf(stdout,pFormat,pvar);
    va_end (pvar);
    printf("\n");
    return(nchar);
}


int simDetectorSetup(int num_cameras)   /* number of simulated cameras in system.  */
{

    if (num_cameras < 1) {
        printf("simDetectorSetup, num_cameras must be > 0\n");
        return AREA_DETECTOR_ERROR;
    }
    numCameras = num_cameras;
    allCameras = (camera_t *)calloc(numCameras, sizeof(camera_t)); 
    return AREA_DETECTOR_OK;
}


int simDetectorConfig(int camera, int maxSizeX, int maxSizeY, int dataType)     /* Camera number */

{
    DETECTOR_HDL pCamera;
    int status = AREA_DETECTOR_OK;

    if (numCameras < 1) {
        printf("simDetectorConfig: no simDetector cameras allocated, call simDetectorSetup first\n");
        return AREA_DETECTOR_ERROR;
    }
    if ((camera < 0) || (camera >= numCameras)) {
        printf("simDetectorConfig: camera must in range 0 to %d\n", numCameras-1);
        return AREA_DETECTOR_ERROR;
    }
    pCamera = &allCameras[camera];
    pCamera->camera = camera;
    
    /* Set the local log function, may be changed by higher layers */
    ADSetLog(pCamera, NULL, NULL);

    /* Initialize the parameter library */
    pCamera->params = ADParam->create(0, ADLastDriverParam);
    if (!pCamera->params) {
        printf("simDetectorConfig: unable to create parameter library\n");
        return AREA_DETECTOR_ERROR;
    }
    
    /* Set some parameters that need to be initialized */
    status =  ADParam->setString(pCamera->params, ADManufacturer, "Simulated detector");
    status |= ADParam->setString(pCamera->params, ADModel, "Basic simulator");
    status |= ADParam->setInteger(pCamera->params, ADStatus, ADStatusIdle);
    status |= ADParam->setInteger(pCamera->params, ADAcquire, 0);
    status |= ADParam->setInteger(pCamera->params, ADMaxSizeX, maxSizeX);
    status |= ADParam->setInteger(pCamera->params, ADMaxSizeY, maxSizeY);
    status |= ADParam->setInteger(pCamera->params, ADSizeX, maxSizeX);
    status |= ADParam->setInteger(pCamera->params, ADSizeY, maxSizeY);
    status |= ADParam->setInteger(pCamera->params, ADImageSizeX, maxSizeX);
    status |= ADParam->setInteger(pCamera->params, ADImageSizeY, maxSizeY);
    status |= ADParam->setInteger(pCamera->params, ADDataType, dataType);
    if (status) {
        printf("simDetectorConfig: unable to set camera parameters\n");
        return AREA_DETECTOR_ERROR;
    }
    
    /* Create the epicsEvent for signaling to the simulate task when acquisition starts */
    pCamera->eventId = epicsEventCreate(epicsEventEmpty);
    
    /* Create the thread that updates the images */
    status = (epicsThreadCreate("SimDetTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)ADSimulateTask,
                                pCamera) == NULL);
    if (status) {
        printf("simDetectorConfig: epicsThreadCreate failure\n");
        return AREA_DETECTOR_ERROR;
    }
    
    return AREA_DETECTOR_OK;
}

/* Code for iocsh registration */

/* simDetectorSetup */
static const iocshArg simDetectorSetupArg0 = {"Number of simulated detectors", iocshArgInt};
static const iocshArg * const simDetectorSetupArgs[1] =  {&simDetectorSetupArg0};
static const iocshFuncDef setupsimDetector = {"simDetectorSetup", 1, simDetectorSetupArgs};
static void setupsimDetectorCallFunc(const iocshArgBuf *args)
{
    simDetectorSetup(args[0].ival);
}


/* simDetectorConfig */
static const iocshArg simDetectorConfigArg0 = {"Camera being configured", iocshArgInt};
static const iocshArg simDetectorConfigArg1 = {"Max X size", iocshArgInt};
static const iocshArg simDetectorConfigArg2 = {"Max Y size", iocshArgInt};
static const iocshArg simDetectorConfigArg3 = {"Data type", iocshArgInt};
static const iocshArg * const simDetectorConfigArgs[4] = {&simDetectorConfigArg0,
                                                          &simDetectorConfigArg1,
                                                          &simDetectorConfigArg2,
                                                          &simDetectorConfigArg3};
static const iocshFuncDef configsimDetector = {"simDetectorConfig", 4, simDetectorConfigArgs};
static void configsimDetectorCallFunc(const iocshArgBuf *args)
{
    simDetectorConfig(args[0].ival, args[1].ival, args[2].ival, args[3].ival);
}


static void simDetectorRegister(void)
{

    iocshRegister(&setupsimDetector,  setupsimDetectorCallFunc);
    iocshRegister(&configsimDetector, configsimDetectorCallFunc);
}

epicsExportRegistrar(simDetectorRegister);

