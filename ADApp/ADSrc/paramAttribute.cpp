/* paramAttribute.cpp
 *
 * \author Mark Rivers
 *
 * \author University of Chicago
 *
 * \date April 30, 2009
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ellLib.h>
#include <epicsMutex.h>
#include <epicsString.h>

#include <asynDriver.h>

#include "NDArray.h"
#include "paramAttribute.h"

static const char *driverName = "paramAttribute";

/* This asynUser is not attached to any device.  
 * It lets one turn on debugging by settings the global asynTrace flag bits
 * ASTN_TRACE_ERROR (default), ASYN_TRACE_FLOW, etc. */

static asynUser *pasynUserSelf = NULL;

/** Constructor for a Param attribute
  */
paramAttribute::paramAttribute(const char *pName, const char *pDescription, const char *pSource, int addr, 
                            class asynNDArrayDriver *pDriver, const char *dataType)
    : NDAttribute(pName)
{
    static const char *functionName = "paramAttribute";
    asynUser *pasynUser;
    int status;
    
    /* Create the static pasynUser if not already done */
    if (!pasynUserSelf) pasynUserSelf = pasynManager->createAsynUser(0,0);
    if (pDescription) this->setDescription(pDescription);
    if (!pSource) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "paramAttribute: ERROR, must specify source string\n",
            driverName, functionName);
        return;
    }
    this->setSource(pSource);
    this->paramType = PVAttrParamUnknown;
    this->sourceType = NDAttrSourceParam;
    this->paramAddr = addr;
    this->pDriver = pDriver;
    pasynUser = pasynManager->createAsynUser(0,0);
    status = this->pDriver->drvUserCreate(pasynUser, pSource, NULL, 0);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, cannot find parameter %s\n",
            driverName, functionName, pSource);
        return;
    }
    this->paramId = pasynUser->reason;
    if (epicsStrCaseCmp(dataType, "int") == 0)    this->paramType=PVAttrParamInt;
    if (epicsStrCaseCmp(dataType, "double") == 0) this->paramType=PVAttrParamDouble;
    if (epicsStrCaseCmp(dataType, "string") == 0) this->paramType=PVAttrParamString;
}

/** Constructor for a Param attribute
  */
paramAttribute::~paramAttribute()
{
}

int paramAttribute::updateValue()
{
    int status = asynSuccess;
    char stringValue[MAX_ATTRIBUTE_STRING_SIZE] = "";
    epicsInt32 i32Value=0;
    epicsFloat64 f64Value=0.;
    static const char *functionName = "updateValue";
    
    switch (this->paramType) {
        case PVAttrParamInt:
            status = this->pDriver->getIntegerParam(this->paramAddr, this->paramId, 
                                                 &i32Value);
            this->setValue(NDAttrInt32, &i32Value);
            break;
        case PVAttrParamDouble:
            status = this->pDriver->getDoubleParam(this->paramAddr, this->paramId,
                                                &f64Value);
            this->setValue(NDAttrFloat64, &f64Value);
            break;
        case PVAttrParamString:
            status = this->pDriver->getStringParam(this->paramAddr, this->paramId,
                                                MAX_ATTRIBUTE_STRING_SIZE, stringValue);
            this->setValue(NDAttrString, stringValue);
            break;
        default:
            break;
    }
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR reading parameter attribute value, name=%s, source=%s, type=%d\n",
            driverName, functionName, this->pName, this->pSource, this->paramType);
    }
    return(status);
}

/** Reports on the properties of the paramAttribute object; 
  * calls NDAttribute::report to report on the EPICS PV value.
  */
int paramAttribute::report(int details)
{
    printf("paramAttribute, address=%p:\n", this);
    printf("  Source type=%d\n", this->sourceType);
    printf("  Source=%s\n", this->pSource);
    printf("  Addr=%d\n", this->paramAddr);
    printf("  Param type=%d\n", this->paramType);
    printf("  Param ID=%d\n", this->paramId);
    NDAttribute::report(details);
    return(ND_SUCCESS);
}
    
