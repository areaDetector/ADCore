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
#include "ADParamLib.h"
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
    
    status |= ADParam->getString(this->params[addr], ADFilePath, sizeof(filePath), filePath); 
    status |= ADParam->getString(this->params[addr], ADFileName, sizeof(fileName), fileName); 
    status |= ADParam->getString(this->params[addr], ADFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= ADParam->getInteger(this->params[addr], ADFileNumber, &fileNumber);
    status |= ADParam->getInteger(this->params[addr], ADAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate, 
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= ADParam->setInteger(this->params[addr], ADFileNumber, fileNumber);
        status |= ADParam->setInteger(this->params[addr], ADFileNumber_RBV, fileNumber);
    }
    return(status);   
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
    

ADDriverBase::ADDriverBase(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory)

    : asynNDArrayBase(portName, maxAddr, paramTableSize, maxBuffers, maxMemory)

{
    //char *functionName = "ADDriverBase";
    paramList *params = this->params[0];

    /* Set some default values for parameters */
    ADParam->setString (params, ADManufacturer_RBV, "Unknown");
    ADParam->setString (params, ADModel_RBV,        "Unknown");
    ADParam->setDouble( params, ADGain_RBV,         1.0);
    ADParam->setInteger(params, ADBinX_RBV,         1);
    ADParam->setInteger(params, ADBinY_RBV,         1);
    ADParam->setInteger(params, ADMinX_RBV,         0);
    ADParam->setInteger(params, ADMinY_RBV,         0);
    ADParam->setInteger(params, ADSizeX_RBV,        0);
    ADParam->setInteger(params, ADSizeY_RBV,        0);
    ADParam->setInteger(params, ADMaxSizeX_RBV,     0);
    ADParam->setInteger(params, ADMaxSizeY_RBV,     0);
    ADParam->setInteger(params, ADReverseX_RBV,     0);
    ADParam->setInteger(params, ADReverseY_RBV,     0);
    ADParam->setInteger(params, ADImageSizeX_RBV,   0);
    ADParam->setInteger(params, ADImageSizeY_RBV,   0);
    ADParam->setInteger(params, ADImageSize_RBV,    0);
    ADParam->setInteger(params, ADDataType_RBV,     0);
    ADParam->setInteger(params, ADImageMode_RBV,    ADImageSingle);
    ADParam->setInteger(params, ADTriggerMode_RBV,  ADTriggerInternal);
    ADParam->setInteger(params, ADNumExposures_RBV, 1);
    ADParam->setInteger(params, ADNumImages_RBV,    1);
    ADParam->setDouble (params, ADAcquireTime_RBV,  1.0);
    ADParam->setDouble (params, ADAcquirePeriod_RBV,1.0);
    ADParam->setInteger(params, ADStatus_RBV,       ADStatusIdle);
    ADParam->setInteger(params, ADShutter_RBV,      0);
    ADParam->setInteger(params, ADAcquire_RBV,      0);
    ADParam->setInteger(params, ADImageCounter_RBV, 0);
    ADParam->setString (params, ADFilePath_RBV,     ".");
    ADParam->setString (params, ADFileName_RBV,     "test");
    ADParam->setInteger(params, ADFileNumber_RBV,   1);
    ADParam->setString (params, ADFileTemplate_RBV, "%s%s_%d.dat");
    ADParam->setString (params, ADFullFileName_RBV, "");
    ADParam->setInteger(params, ADAutoIncrement_RBV,1);
    ADParam->setInteger(params, ADFileFormat_RBV,   0);
    ADParam->setInteger(params, ADAutoSave_RBV,     0);
    ADParam->setInteger(params, ADWriteFile_RBV,    0);
    ADParam->setInteger(params, ADReadFile_RBV,     0);
}
