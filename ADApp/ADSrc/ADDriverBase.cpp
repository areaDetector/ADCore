/* ADDriverBase.c
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
#include "ADDriverBase.h"


static char *driverName = "ADDriverBase";

int ADDriverBase::createFileName(int maxChars, char *fullFileName)
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
        status |= setIntegerParam(addr, ADFileNumber_RBV, fileNumber);
    }
    return(status);   
}
asynStatus ADDriverBase::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) setIntegerParam(addr, function+1, value);

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(addr, addr);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    return status;
}

asynStatus ADDriverBase::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr=0;
    const char *functionName = "writeFloat64";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
 
    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(addr, function, value);
    status = setDoubleParam(addr, function+1, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(addr, addr);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    return status;
}


asynStatus ADDriverBase::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int addr=0;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) setStringParam(addr, function+1, (char *)value);

     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)callParamCallbacks(addr, addr);

    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    return status;
}



/* asynDrvUser routines */
asynStatus ADDriverBase::drvUserCreate(asynUser *pasynUser,
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
    

ADDriverBase::ADDriverBase(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                           int interfaceMask, int interruptMask)

    : asynNDArrayBase(portName, maxAddr, paramTableSize, maxBuffers, maxMemory,
          interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynHandleMask | asynDrvUserMask,
          interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynHandleMask)

{
    //char *functionName = "ADDriverBase";

    /* Set some default values for parameters */
    setStringParam (ADManufacturer_RBV, "Unknown");
    setStringParam (ADModel_RBV,        "Unknown");
    setDoubleParam (ADGain_RBV,         1.0);
    setIntegerParam(ADBinX_RBV,         1);
    setIntegerParam(ADBinY_RBV,         1);
    setIntegerParam(ADMinX_RBV,         0);
    setIntegerParam(ADMinY_RBV,         0);
    setIntegerParam(ADSizeX_RBV,        0);
    setIntegerParam(ADSizeY_RBV,        0);
    setIntegerParam(ADMaxSizeX_RBV,     0);
    setIntegerParam(ADMaxSizeY_RBV,     0);
    setIntegerParam(ADReverseX_RBV,     0);
    setIntegerParam(ADReverseY_RBV,     0);
    setIntegerParam(ADImageSizeX_RBV,   0);
    setIntegerParam(ADImageSizeY_RBV,   0);
    setIntegerParam(ADImageSize_RBV,    0);
    setIntegerParam(ADDataType_RBV,     0);
    setIntegerParam(ADImageMode_RBV,    ADImageSingle);
    setIntegerParam(ADTriggerMode_RBV,  ADTriggerInternal);
    setIntegerParam(ADNumExposures_RBV, 1);
    setIntegerParam(ADNumImages_RBV,    1);
    setDoubleParam (ADAcquireTime_RBV,  1.0);
    setDoubleParam (ADAcquirePeriod_RBV,1.0);
    setIntegerParam(ADStatus_RBV,       ADStatusIdle);
    setIntegerParam(ADShutter_RBV,      0);
    setIntegerParam(ADAcquire_RBV,      0);
    setIntegerParam(ADImageCounter_RBV, 0);
    setStringParam (ADFilePath_RBV,     ".");
    setStringParam (ADFileName_RBV,     "test");
    setIntegerParam(ADFileNumber_RBV,   1);
    setStringParam (ADFileTemplate_RBV, "%s%s_%d.dat");
    setStringParam (ADFullFileName_RBV, "");
    setIntegerParam(ADAutoIncrement_RBV,1);
    setIntegerParam(ADFileFormat_RBV,   0);
    setIntegerParam(ADAutoSave_RBV,     0);
    setIntegerParam(ADWriteFile_RBV,    0);
    setIntegerParam(ADReadFile_RBV,     0);
}
