/* functAttribute.cpp
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
#include <registryFunction.h>

#include <asynDriver.h>

#define epicsExportSharedSymbols
#include <shareLib.h>
#include <asynNDArrayDriver.h>
#include "NDArray.h"
#include "functAttribute.h"

static const char *driverName = "functAttribute";

/* This asynUser is not attached to any device.  
 * It lets one turn on debugging by settings the global asynTrace flag bits
 * ASTN_TRACE_ERROR (default), ASYN_TRACE_FLOW, etc. */

static asynUser *pasynUserSelf = NULL;

/** Constructor for function attribute
  * \param[in] pName The name of the attribute to be created; case-insensitive. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] pSource The symbol name for the function to be called to get the value of the parameter.
  * \param[in] pParam A string that will be passed to the function, typically to specify what/how to get the value.
  */
functAttribute::functAttribute(const char *pName, const char *pDescription, const char *pSource, const char *pParam)

    : NDAttribute(pName, pDescription, NDAttrSourceFunct, pSource, NDAttrUndefined, 0),
      pFunction(0), functionPvt(0)
{
    static const char *functionName = "functAttribute";
    
    /* Create the static pasynUser if not already done */
    if (!pasynUserSelf) pasynUserSelf = pasynManager->createAsynUser(0,0);
    if (pParam) 
        functParam = epicsStrDup(pParam);
    else
        functParam = epicsStrDup("");
    if (!pSource) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must specify function name\n",
            driverName, functionName);
        goto error;
    }
    this->pFunction = (NDAttributeFunction)registryFunctionFind(pSource);
    if (!this->pFunction) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, cannot find function %s\n",
            driverName, functionName, pSource);
        goto error;
    }
    
error:
    return;
}

/** Copy constructor for function attribute
  * \param[in] attribute The functAttribute to copy from.
  */
functAttribute::functAttribute(functAttribute& attribute)

    : NDAttribute(attribute)
{
    functParam = epicsStrDup(attribute.functParam);
    pFunction = attribute.pFunction;
    functionPvt = attribute.functionPvt;
}

/** Destructor for driver/plugin attribute
  */
functAttribute::~functAttribute()
{
    free(functParam);
}

functAttribute* functAttribute::copy(NDAttribute *pAttr)
{
  functAttribute *pOut = (functAttribute *)pAttr;
  if (!pOut) 
    pOut = new functAttribute(*this);
  else {
    // NOTE: We assume that if the attribute name is the same then the function name and parameter are also the same
    // are also the same
    NDAttribute::copy(pOut);
  }
  return(pOut);
}


/** Updates the current value of this attribute; sets the attribute value to the return value of the
  * specified function.
  */
int functAttribute::updateValue()
{
    //static const char *functionName = "updateValue";
    
    if (!this->pFunction) return asynError;
    
    this->pFunction(this->functParam, &functionPvt, this);
    return asynSuccess;
}


/** Reports on the properties of the functAttribute object; 
  * calls base class NDAttribute::report() to report on the parameter value.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; currently does nothing in this derived class.
  */
int functAttribute::report(FILE *fp, int details)
{
    NDAttribute::report(fp, details);
    fprintf(fp, "  functAttribute\n");
    fprintf(fp, "    functParam=%s\n", this->functParam);
    fprintf(fp, "    pFunction=%p\n", this->pFunction);
    fprintf(fp, "    functionPvt=%p\n", this->functionPvt);
    return(ND_SUCCESS);
}
    
