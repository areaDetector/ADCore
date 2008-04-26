/* ADUtils.h
 *
 * This file provides some utility functions that can be used by area detector
 * drivers.
 *
 * Mark Rivers
 * University of Chicago
 * March 5, 2008
 *
 */

#include <string.h>
#include <stdio.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include "ADUtils.h"
#include "ADParamLib.h"
#include "ADInterface.h"
#include "asynHandle.h"

/* static char *driverName = "ADUtils"; */

static int setParamDefaults( void *params)
{
    /* Set some reasonable defaults for some parameters */
    int status = AREA_DETECTOR_OK;
    
    status |= ADParam->setString (params, ADManufacturer, "Unknown");
    status |= ADParam->setString (params, ADModel,        "Unknown");
    status |= ADParam->setInteger(params, ADMinY,         0);
    status |= ADParam->setInteger(params, ADMinX,         0);
    status |= ADParam->setInteger(params, ADMinY,         0);
    status |= ADParam->setInteger(params, ADBinX,         1);
    status |= ADParam->setInteger(params, ADBinY,         1);
    status |= ADParam->setDouble( params, ADGain,         1.0);
    status |= ADParam->setInteger(params, ADStatus,       ADStatusIdle);
    status |= ADParam->setInteger(params, ADAcquire,      0);
    status |= ADParam->setDouble (params, ADAcquireTime,  1.0);
    status |= ADParam->setInteger(params, ADNumExposures, 1);
    status |= ADParam->setInteger(params, ADNumImages,    1);
    status |= ADParam->setInteger(params, ADImageMode,    ADImageSingle);
    status |= ADParam->setInteger(params, ADImageCounter, 0);
    status |= ADParam->setInteger(params, ADTriggerMode,  ADTriggerInternal);
    status |= ADParam->setInteger(params, ADFileNumber,   1);
    status |= ADParam->setInteger(params, ADAutoIncrement,1);
    status |= ADParam->setInteger(params, ADAutoSave,     0);
    status |= ADParam->setInteger(params, ADWriteFile,    0);
    status |= ADParam->setInteger(params, ADReadFile,     0);
    status |= ADParam->setInteger(params, ADFileFormat,   0);

    return status;
}


static int createFileName(void *params, int maxChars, char *fullFileName)
{
    /* Formats a complete file name from the components defined in ADInterface.h */
    int status = AREA_DETECTOR_OK;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    char fileTemplate[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    status |= ADParam->getString(params, ADFilePath, sizeof(filePath), filePath); 
    status |= ADParam->getString(params, ADFileName, sizeof(fileName), fileName); 
    status |= ADParam->getString(params, ADFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= ADParam->getInteger(params, ADFileNumber, &fileNumber);
    status |= ADParam->getInteger(params, ADAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate, 
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= AREA_DETECTOR_ERROR;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= ADParam->setInteger(params, ADFileNumber, fileNumber);
    }
    return(status);   
}


static void handleCallback(void *handelInterruptPvt, void *handle, int reason, int addr)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int address;

    pasynManager->interruptStart(handelInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynHandleInterrupt *pInterrupt = pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((reason == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 handle);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(handelInterruptPvt);
}


static void dimensionCallback(void *int32ArrayInterruptPvt, epicsInt32 *dimsPrev, 
                              NDArray_t *pArray, int reason)
{
    int i, dimsChanged;
    ELLLIST *pclientList;
    interruptNode *pnode;
    
    for (i=0, dimsChanged=0; i<pArray->ndims; i++) {
        if (pArray->dims[i].size != dimsPrev[i]) {
            dimsPrev[i] = pArray->dims[i].size;
            dimsChanged = 1;
        }
    }
    if (dimsChanged) {
        pasynManager->interruptStart(int32ArrayInterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32ArrayInterrupt *pInterrupt = pnode->drvPvt;
            if (pInterrupt->pasynUser->reason == reason) {
                pInterrupt->callback(pInterrupt->userPvt,
                                     pInterrupt->pasynUser,
                                     dimsPrev, pArray->ndims);
                pnode = (interruptNode *)ellNext(&pnode->node);
            }
        }
        pasynManager->interruptEnd(int32ArrayInterruptPvt);
    }
}

static int findParam(ADParamString_t *paramTable, int numParams, const char *paramName, int *param)
{
    int i;
    for (i=0; i < numParams; i++) {
        if (epicsStrCaseCmp(paramName, paramTable[i].paramString) == 0) {
            *param = paramTable[i].param;
            return(AREA_DETECTOR_OK);
        }
    }
    return(AREA_DETECTOR_ERROR);
}


static ADUtilsSupport utilsSupport =
{
    setParamDefaults,
    createFileName,
    handleCallback,
    dimensionCallback,
    findParam
};

ADUtilsSupport *ADUtils = &utilsSupport;
