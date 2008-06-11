/* ADDriver.c
 *
 * This is a driver for a simulated area detector.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */
 
#include <stdio.h>

#include <epicsString.h>
#include <asynStandardInterfaces.h>

/* Defining this will create the static table of standard parameters in ADInterface.h */
#define DEFINE_AD_STANDARD_PARAMS 1
#include "ADStdDriverParams.h"
#include "ADDriver.h"


static char *driverName = "ADDriver";

int ADDriver::createFileName(int maxChars, char *fullFileName)
{
    /* Formats a complete file name from the components defined in ADStdDriverParams.h */
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    char fileTemplate[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    int addr=0;
    
    status |= getStringParam(addr, ADFilePath, sizeof(filePath), filePath); 
    status |= getStringParam(addr, ADFileName, sizeof(fileName), fileName); 
    status |= getStringParam(addr, ADFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= getIntegerParam(addr, ADFileNumber, &fileNumber);
    status |= getIntegerParam(addr, ADAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate, 
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(addr, ADFileNumber, fileNumber);
    }
    return(status);   
}

/* asynDrvUser routines */
asynStatus ADDriver::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    const char *functionName = "drvUserCreate";

    /* See if this is one of the standard parameters */
    status = findParam(ADStdDriverParamString, NUM_AD_STANDARD_PARAMS, 
                       drvInfo, &param);
                                
    if (status == asynSuccess) {
        pasynUser->reason = param;
        if (pptypeName) {
            *pptypeName = epicsStrDup(drvInfo);
        }
        if (psize) {
            *psize = sizeof(param);
        }
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
                  "%s:%s:, drvInfo=%s, param=%d\n", 
                  driverName, functionName, drvInfo, param);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s:%s:, unknown drvInfo=%s", 
                     driverName, functionName, drvInfo);
        return(asynError);
    }
}
    

ADDriver::ADDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                   int interfaceMask, int interruptMask)

    : asynNDArrayDriver(portName, maxAddr, paramTableSize, maxBuffers, maxMemory,
          interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask | asynDrvUserMask,
          interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask)

{
    //char *functionName = "ADDriver";

    /* Set some default values for parameters */
    setDoubleParam (ADGain,         1.0);
    setIntegerParam(ADBinX,         1);
    setIntegerParam(ADBinY,         1);
    setIntegerParam(ADMinX,         0);
    setIntegerParam(ADMinY,         0);
    setIntegerParam(ADSizeX,        1);
    setIntegerParam(ADSizeY,        1);
    setIntegerParam(ADReverseX,     0);
    setIntegerParam(ADReverseY,     0);
    setIntegerParam(ADTriggerMode,  0);
    setIntegerParam(ADNumExposures, 1);
    setIntegerParam(ADStatus,       ADStatusIdle);
    setIntegerParam(ADAcquire,      0);
    setIntegerParam(ADImageCounter, 0);
    
    setStringParam (ADFilePath,     "");
    setStringParam (ADFileName,     "");
    setIntegerParam(ADFileNumber,   0);
    setStringParam (ADFileTemplate, "");
    setIntegerParam(ADAutoIncrement, 0);
    setStringParam (ADFullFileName, "");
    setIntegerParam(ADFileFormat,   0);
    setIntegerParam(ADAutoSave,     0);
    setIntegerParam(ADWriteFile,    0);
    setIntegerParam(ADReadFile,     0);
}
