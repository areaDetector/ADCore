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


static ADUtilsSupport utilsSupport =
{
    setParamDefaults,
    createFileName,
};

ADUtilsSupport *ADUtils = &utilsSupport;
