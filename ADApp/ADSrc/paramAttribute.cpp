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

#define epicsExportSharedSymbols
#include <shareLib.h>
#include <asynNDArrayDriver.h>
#include "NDArray.h"
#include "paramAttribute.h"

static const char *driverName = "paramAttribute";

/* This asynUser is not attached to any device.  
 * It lets one turn on debugging by settings the global asynTrace flag bits
 * ASTN_TRACE_ERROR (default), ASYN_TRACE_FLOW, etc. */

static asynUser *pasynUserSelf = NULL;

/** Constructor for driver/plugin attribute
  * \param[in] pName The name of the attribute to be created; case-insensitive. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] pSource The DRV_INFO string used to identify the parameter in the asynPortDriver.
  * \param[in] addr The asyn addr (address) for this parameter.
  * \param[in] pDriver The driver or plugin object from which to obtain the parameter.
  * \param[in] dataType The data type for this parameter.  Must be "INT", "DOUBLE", or "STRING" (case-insensitive).
  */
paramAttribute::paramAttribute(const char *pName, const char *pDescription, const char *pSource, int addr, 
                               class asynNDArrayDriver *pDriver, const char *dataType)
    : NDAttribute(pName, pDescription, NDAttrSourceParam, pSource, NDAttrUndefined, 0),
    paramAddr(addr), paramType(paramAttrTypeUnknown), pDriver(pDriver)
{
    static const char *functionName = "paramAttribute";
    asynUser *pasynUser=NULL;
    int status;
    
    /* Create the static pasynUser if not already done */
    if (!pasynUserSelf) pasynUserSelf = pasynManager->createAsynUser(0,0);
    if (!pSource) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must specify source string\n",
            driverName, functionName);
        goto error;
    }
    pasynUser = pasynManager->createAsynUser(0,0);
    status = pasynManager->connectDevice(pasynUser, pDriver->portName, this->paramAddr);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, cannot connect to driver for parameter %s\n",
            driverName, functionName, pSource);
        goto error;
    }
    status = this->pDriver->drvUserCreate(pasynUser, pSource, NULL, 0);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, cannot find parameter %s\n",
            driverName, functionName, pSource);
        goto error;
    }
    this->paramId = pasynUser->reason;
    status = pasynManager->disconnect(pasynUser);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, cannot disconnect from driver for parameter %s\n",
            driverName, functionName, pSource);
        goto error;
    }
    if      (!strcmp(dataType, "INT")) {
        this->paramType=paramAttrTypeInt;
        this->setDataType(NDAttrInt32);
    }
    else if (!strcmp(dataType, "DOUBLE")) {
        this->paramType=paramAttrTypeDouble;
        this->setDataType(NDAttrFloat64);
    }
    else if (!strcmp(dataType, "STRING")) {
        this->paramType=paramAttrTypeString;
        this->setDataType(NDAttrString);
    }

error:
    if (pasynUser) pasynManager->freeAsynUser(pasynUser);
}

/** Copy constructor for driver/plugin attribute
  * \param[in] attribute A paramAttribute to copy from 
  */
paramAttribute::paramAttribute(paramAttribute& attribute)
    : NDAttribute(attribute)
{
    paramType = attribute.paramType;
    paramAddr = attribute.paramAddr;
    pDriver = attribute.pDriver;
    paramId = attribute.paramId;
}

/** Destructor for driver/plugin attribute
  */
paramAttribute::~paramAttribute()
{
}

/** Updates the current value of this attribute; sets the attribute value to the current value of the
  * driver/plugin parameter in the parameter library.
  */
int paramAttribute::updateValue()
{
    int status = asynSuccess;
    char stringValue[MAX_ATTRIBUTE_STRING_SIZE] = "";
    epicsInt32 i32Value=0;
    epicsFloat64 f64Value=0.;
    static const char *functionName = "updateValue";
    
    switch (this->paramType) {
        case paramAttrTypeInt:
            status = this->pDriver->getIntegerParam(this->paramAddr, this->paramId, 
                                                 &i32Value);
            this->setValue(&i32Value);
            break;
        case paramAttrTypeDouble:
            status = this->pDriver->getDoubleParam(this->paramAddr, this->paramId,
                                                &f64Value);
            this->setValue(&f64Value);
            break;
        case paramAttrTypeString:
            status = this->pDriver->getStringParam(this->paramAddr, this->paramId,
                                                MAX_ATTRIBUTE_STRING_SIZE, stringValue);
            this->setValue(stringValue);
            break;
        default:
            break;
    }
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR reading parameter attribute value, name=%s, source=%s, type=%d\n",
            driverName, functionName, this->getName(), this->getSource(), this->paramType);
    }
    return(status);
}

paramAttribute* paramAttribute::copy(NDAttribute *pAttr)
{
  paramAttribute *pOut = (paramAttribute *)pAttr;
  if (!pOut) 
    pOut = new paramAttribute(*this);
  else {
    NDAttribute::copy(pOut);
  }
  return(pOut);
}


/** Reports on the properties of the paramAttribute object; 
  * calls base class NDAttribute::report() to report on the parameter value.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; currently does nothing in this derived class.
  */
int paramAttribute::report(FILE *fp, int details)
{
    NDAttribute::report(fp, details);
    fprintf(fp, "  paramAttribute\n");
    fprintf(fp, "    Addr=%d\n", this->paramAddr);
    fprintf(fp, "    Param type=%d\n", this->paramType);
    fprintf(fp, "    Param ID=%d\n", this->paramId);
    return(ND_SUCCESS);
}
    
