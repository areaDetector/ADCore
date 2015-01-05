/* PVAttribute.cpp
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
#include <cadef.h>
#include <epicsEvent.h>

#include <asynDriver.h>

#define epicsExportSharedSymbols
#include <shareLib.h>
#include "PVAttribute.h"

static const char *driverName = "PVAttribute";
static ca_client_context *pCaInputContext=NULL;
static void connectCallbackC(struct connection_handler_args cha);

/* This asynUser is not attached to any device.  
 * It lets one turn on debugging by settings the global asynTrace flag bits
 * ASTN_TRACE_ERROR (default), ASYN_TRACE_FLOW, etc. */

static asynUser *pasynUserSelf = NULL;

/** Constructor for an EPICS PV attribute
  * \param[in] pName The name of the attribute to be created; case-insensitive. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] pSource The name of the EPICS PV to be used to obtain the attribute value.
  * \param[in] dbrType The EPICS DBR_XXX type to be used (DBR_STRING, DBR_DOUBLE, etc).
  *                    In addition to the normal DBR types a special type, DBR_NATIVE, may be used,
  *                    which means to use the native data type returned by Channel Access for this PV.
  */
PVAttribute::PVAttribute(const char *pName, const char *pDescription,
                         const char *pSource, chtype dbrType)
    : NDAttribute(pName, pDescription, NDAttrSourceEPICSPV, pSource, NDAttrUndefined, 0),
    dbrType(dbrType), callbackString(0)
{
    static const char *functionName = "PVAttribute";
    
    /* Create the static pasynUser if not already done */
    if (!pasynUserSelf) pasynUserSelf = pasynManager->createAsynUser(0,0);

    /* Create the ca_context if not already done */
    if (pCaInputContext == NULL) {
        SEVCHK(ca_context_create(ca_enable_preemptive_callback),"ca_context_create");
        while (pCaInputContext == NULL) {
            epicsThreadSleep(epicsThreadSleepQuantum());
            pCaInputContext = ca_current_context();
        }
    }
    /* Need to attach to the ca_context because this method could be called in a different thread from
     * that which created the context */
    ca_attach_context(pCaInputContext);
    this->lock = epicsMutexCreate();
    if (!pSource) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must specify source string\n",
            driverName, functionName);
        return;
    }
    /* Set connection callback on this PV */
    SEVCHK(ca_create_channel(pSource, connectCallbackC, this, 10 ,&this->chanId),
           "ca_create_channel");
}

/** Copy constructor for an EPICS PV attribute
  * \param[in] attribute The PVAttribute to copy from.
  * NOTE: The copy is not "functional", i.e. it does not update when the PV changes, the value is frozen. 
  */
PVAttribute::PVAttribute(PVAttribute& attribute)
    : NDAttribute(attribute)
{
    dbrType = attribute.dbrType;
    eventId = 0;
    chanId = 0;
    lock = 0;
}


PVAttribute::~PVAttribute()
{
    if (this->chanId) SEVCHK(ca_clear_channel(this->chanId),"ca_clear_channel");
    if (this->lock) epicsMutexDestroy(this->lock);
}


PVAttribute* PVAttribute::copy(NDAttribute *pAttr)
{
  PVAttribute *pOut = (PVAttribute *)pAttr;
  if (!pOut) 
    pOut = new PVAttribute(*this);
  else {
    // NOTE: We assume that if the attribute name is the same then the source PV and dbrTtype
    // are also the same
    NDAttribute::copy(pOut);
  }
  return(pOut);
}

static void monitorCallbackC(struct event_handler_args cha)
{
    PVAttribute *pAttribute = (PVAttribute *)ca_puser(cha.chid);
    if (!pAttribute) return;
    pAttribute->monitorCallback(cha);
}

/** Monitor callback called whenever an EPICS PV changes value.
  * Calls NDAttribute::setValue to store the new value.
  * \param[in] eha Event handler argument structure passed by channel access. 
  */
void PVAttribute::monitorCallback(struct event_handler_args eha)
{
    //chid  chanId = eha.chid;
    NDAttrDataType_t dataType = this->getDataType();
    const char *functionName = "monitorCallback";

    epicsMutexLock(this->lock);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s:%s: PV=%s\n", 
        driverName, functionName, this->getSource());

    if (eha.status != ECA_NORMAL) {
        asynPrint(pasynUserSelf,  ASYN_TRACE_ERROR,
        "%s:%s: CA returns eha.status=%d\n",
        driverName, functionName, eha.status);
        goto done;
    }
    /* Treat strings specially */
    if (dataType == NDAttrString) {
      if (this->callbackString) free(this->callbackString);
      this->callbackString = epicsStrDup((char *)eha.dbr);
      goto done;
    }
    switch (dataType) {
      case NDAttrInt8:
        callbackValue.i8 = *(epicsInt8 *)eha.dbr;
        break;
      case NDAttrUInt8:
        callbackValue.ui8 = *(epicsUInt8 *)eha.dbr;
        break;
      case NDAttrInt16:
        callbackValue.i16 = *(epicsInt16 *)eha.dbr;
        break;
      case NDAttrUInt16:
        callbackValue.ui16 = *(epicsUInt16 *)eha.dbr;
        break;
      case NDAttrInt32:
        callbackValue.i32 = *(epicsInt32*)eha.dbr;
        break;
      case NDAttrUInt32:
        callbackValue.ui32 = *(epicsUInt32 *)eha.dbr;
        break;
      case NDAttrFloat32:
        callbackValue.f32 = *(epicsFloat32 *)eha.dbr;
        break;
      case NDAttrFloat64:
        callbackValue.f64 = *(epicsFloat64 *)eha.dbr;
        break;
      case NDAttrUndefined:
        break;
      default:
        break;
    }
    done:
    epicsMutexUnlock(this->lock);
}

int PVAttribute::updateValue()
{
    //static const char *functionName = "updateValue"
    
    void *pValue;
    NDAttrDataType_t dataType = this->getDataType();
    
    epicsMutexLock(this->lock);
    if (dataType == NDAttrString)
        pValue = callbackString;
    else
        pValue = &callbackValue;
    this->setValue(pValue);
    epicsMutexUnlock(this->lock);
    return asynSuccess;
}



static void connectCallbackC(struct connection_handler_args cha)
{
    PVAttribute    *pAttribute = (PVAttribute *)ca_puser(cha.chid);
    if (!pAttribute) return;
    pAttribute->connectCallback(cha);
}

/** Connection callback called whenever an EPICS PV connects or disconnects.
  * If it is a connection event it calls ca_add_masked_array_event to request
  * callbacks whenever the value changes.
  * \param[in] cha Connection handler argument structure passed by channel access. 
  */
void PVAttribute::connectCallback(struct connection_handler_args cha)
{
    chid  chanId = cha.chid;
    const char *functionName = "connectCallback";
    chtype dbfType, dbrType=this->dbrType;
    int nRequest=1;
    int elementCount;
    NDAttrDataType_t dataType;
    
    epicsMutexLock(this->lock);
    if (chanId && (ca_state(chanId) == cs_conn)) {
        dbfType = ca_field_type(chanId);
        elementCount = ca_element_count(chanId);
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Connect event, PV=%s, chanId=%p, dbfType=%ld, elementCount=%d, dbrType=%ld\n", 
            driverName, functionName, this->getSource(), chanId, dbfType, elementCount, dbrType);
        switch(dbfType) {
            case DBF_STRING:
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_STRING;
                break;
            case DBF_SHORT:  /* Note: DBF_INT is same as DBF_SHORT */
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_SHORT;
                break;
            case DBF_FLOAT:
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_FLOAT;
                break;
            case DBF_ENUM:
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_SHORT;
                break;
            case DBF_CHAR:
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_CHAR;
                /* If the native type is DBF_CHAR but the requested type is DBR_STRING
                 * read a char array and treat as string */
                if (this->dbrType == DBR_STRING) {
                    dbrType = DBR_CHAR;
                    nRequest = elementCount;
                }
                break;
            case DBF_LONG:
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_LONG;
                break;
            case DBF_DOUBLE:
                if (this->dbrType == DBR_NATIVE) dbrType = DBR_DOUBLE;
                break;
            default:
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unknown DBF type = %ld\n",
                driverName, functionName, dbfType);
                goto done;
        }
        switch(dbrType) {
            case DBR_STRING:
                dataType = NDAttrString;
                break;
            case DBR_SHORT:
                dataType = NDAttrInt16;
                break;
            case DBR_FLOAT:
                dataType = NDAttrFloat32;
                break;
            case DBR_ENUM:
                dataType = NDAttrInt16;
                break;
            case DBR_CHAR:
                dataType = NDAttrInt8;
                /* If the dbrType is DBR_CHAR but the requested type is DBR_STRING
                 * read a char array and treat as string */
                if (this->dbrType == DBR_STRING) dataType = NDAttrString;
                break;
            case DBR_LONG:
                dataType = NDAttrInt32;
                break;
            case DBR_DOUBLE:
                dataType = NDAttrFloat64;
                break;
            default:
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unknown DBR type = %ld\n",
                driverName, functionName, dbrType);
                goto done;
        }
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Connect event, PV=%s, chanId=%p, type=%d\n", 
            driverName, functionName, this->getSource(), chanId, dataType);
        this->setDataType(dataType);
            
        /* Set value change callback on this PV */
        SEVCHK(ca_add_masked_array_event(
            dbrType,
            nRequest,
            this->chanId,
            monitorCallbackC,
            this,
            0.0,0.0,0.0,
            &this->eventId, DBE_VALUE),"ca_add_masked_array_event");
    } else {
        /* This is a disconnection event */
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Disconnect event, PV=%s, chanId=%p\n", 
            driverName, functionName, this->getSource(), chanId);
    }
    done:
    epicsMutexUnlock(this->lock);
}

/** Reports on the properties of the PVAttribute object; 
  * calls base class NDAttribute::report() to report on the parameter value.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; currently does nothing in this derived class.
  */
int PVAttribute::report(FILE *fp, int details)
{
    NDAttribute::report(fp, details);
    fprintf(fp, "  PVAttribute\n");
    fprintf(fp, "    dbrType=%s\n", dbr_type_to_text(this->dbrType));
    fprintf(fp, "    chanId=%p\n", this->chanId);
    fprintf(fp, "    eventId=%p\n", this->eventId);
    return(ND_SUCCESS);
}
    
