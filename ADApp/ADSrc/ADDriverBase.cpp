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
#include "ADInterface.h"
#include "ADUtils.h"
#include "ADParamLib.h"
#include "ADDriverBase.h"


static char *driverName = "ADDriverBase";


/* asynDrvUser routines */
asynStatus ADDriverBase::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    const char *functionName = "drvUserCreate";

    /* See if this is one of the standard parameters */
    status = findParam(ADStandardParamString, NUM_AD_STANDARD_PARAMS, 
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
    

ADDriverBase::ADDriverBase(const char *portName, int maxAddr, int paramTableSize)

    : asynParamBase(portName, maxAddr, paramTableSize)

{
    //char *functionName = "ADDriverBase";
    int addr = 0;

    /* Set some default values for parameters */
    ADParam->setString (this->params[addr], ADManufacturer, "Unknown");
    ADParam->setString (this->params[addr], ADModel, "Unknown");
}
