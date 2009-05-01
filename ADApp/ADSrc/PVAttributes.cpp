/* PVAttributes.cpp
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
#include "PVAttributes.h"

static const char *driverName = "PVAttributes";
/* This asynUser is not attached to any device.  
 * It lets one turn on debugging by settings the global asynTrace flag bits
 * ASTN_TRACE_ERROR (default), ASYN_TRACE_FLOW, etc. */
static asynUser *pasynUserSelf = pasynManager->createAsynUser(0,0);

static void monitorCallbackC(struct event_handler_args cha)
{
	PVAttr	*pAttr = (PVAttr *)ca_puser(cha.chid);
	if (!pAttr) return;
    pAttr->monitorCallback(cha);
}

/** Monitor callback called whenever an EPICS PV changes value.
  * Calls NDAttribute::setValue to store the new value.
  */
void PVAttr::monitorCallback(struct event_handler_args eha)
{
	//chid  chanId = eha.chid;
    const char *functionName = "monitorCallback";

    epicsMutexLock(this->lock);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s:%s: PV=%s\n", 
        driverName, functionName, this->PVName);

    if (eha.status != ECA_NORMAL) {
		asynPrint(pasynUserSelf,  ASYN_TRACE_ERROR,
        "%s:%s: CA returns eha.status=%d\n",
        driverName, functionName, eha.status);
		goto done;
	}
    this->pNDAttribute->setValue(this->dataType, (void *)eha.dbr);
    done:
    epicsMutexUnlock(this->lock);
}


static void connectCallbackC(struct connection_handler_args cha)
{
	PVAttr	*pAttr = (PVAttr *)ca_puser(cha.chid);
	if (!pAttr) return;
    pAttr->connectCallback(cha);
}

/** Connection callback called whenever an EPICS PV connects or disconnects.
  * If it is a connection event it calls ca_add_masked_array_event to request
  *  callbacks whenever the value changes.
  */
void PVAttr::connectCallback(struct connection_handler_args cha)
{
	chid  chanId = cha.chid;
    const char *functionName = "connectCallback";
    enum channel_state state = cs_never_conn;
    chtype dbfType, dbrType=this->dbrType;
    int nRequest=1;
    int elementCount;
    
    epicsMutexLock(this->lock);
    if (chanId) state = ca_state(chanId);

	if (chanId && (ca_state(chanId) == cs_conn)) {
        dbfType = ca_field_type(chanId);
        elementCount = ca_element_count(chanId);
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
                "%s:%s: unknown DBF type = %d\n",
                driverName, functionName, dbfType);
		        goto done;
        }
        switch(dbrType) {
            case DBR_STRING:
                this->dataType = NDAttrString;
                break;
            case DBR_SHORT:
                this->dataType = NDAttrInt16;
                break;
            case DBR_FLOAT:
                this->dataType = NDAttrFloat32;
                break;
            case DBR_ENUM:
                this->dataType = NDAttrInt16;
                break;
            case DBR_CHAR:
                this->dataType = NDAttrInt8;
                /* If the dbrType is DBR_CHAR but the requested type is DBR_STRING
                 * read a char array and treat as string */
                if (this->dbrType == DBR_STRING) this->dataType = NDAttrString;
                break;
            case DBR_LONG:
                this->dataType = NDAttrInt32;
                break;
            case DBR_DOUBLE:
                this->dataType = NDAttrFloat64;
                break;
            default:
		        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unknown DBR type = %d\n",
                driverName, functionName, dbrType);
		        goto done;
        }
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Connect event, PV=%s, chanId=%d, type=%d\n", 
            driverName, functionName, this->PVName, chanId, this->dataType);
            
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
        /* This is a disconnection event, set the data type to undefined */
		this->pNDAttribute->setValue(NDAttrUndefined, 0);
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Disconnect event, PV=%s, chanId=%d\n", 
            driverName, functionName, this->PVName, chanId);
	}
    done:
    epicsMutexUnlock(this->lock);
}

/** Reports on the properties of the PVAttributes object; 
  * calls NDAttribute::report to report on the EPICS PV values.
  */
int PVAttr::report(int details)
{
    epicsMutexLock(this->lock);
    printf("PVAttr, address=%p:\n", this);
    printf("  PVName=%s\n", this->PVName);
    printf("  dbrType=%s\n", dbr_type_to_text(this->dbrType));
    this->pNDAttribute->report(details);
    epicsMutexUnlock(this->lock);
    return(ND_SUCCESS);
}
    
/** PVAttr constructor
  */
PVAttr::PVAttr(const char *pName, const char *pDescription)
{
    this->PVName = epicsStrDup(pName);
    this->pNDAttribute = new NDAttribute(pName);
    if (pDescription) this->pNDAttribute->setDescription(pDescription);  
    /* Set connection callback on this PV */
	SEVCHK(ca_create_channel(pName, connectCallbackC, this, 10 ,&this->chanId),
	       "ca_create_channel");
    
}
PVAttr::~PVAttr()
{
	if (this->chanId) SEVCHK(ca_clear_channel(this->chanId),"ca_clear_channel");
    if (this->pNDAttribute) delete this->pNDAttribute;
    if (this->PVName) free(this->PVName);
}

/** PVAttributes constructor
  */
PVAttributes::PVAttributes()
{
    ellInit(&this->PVAttrList);
    this->lock = epicsMutexCreate();
    
	SEVCHK(ca_context_create(ca_enable_preemptive_callback),"ca_context_create");
	this->pCaInputContext = ca_current_context();
	while (this->pCaInputContext == NULL) {
		epicsThreadSleep(epicsThreadSleepQuantum());
		this->pCaInputContext = ca_current_context();
	}
}

/** PVAttributes destructor
  */
PVAttributes::~PVAttributes()
{
    this->clearPVs();
    ellFree(&this->PVAttrList);
    epicsMutexDestroy(this->lock);
}

/** Adds a new PV to the PV list, puts connect and monitor callbacks on the PV.
  * \param[in] pName The PV name of the PV. 
  * \param[in] pDescription Description of the PV.
  * \param[in] dbrType DBR_XXX type or DBR_NATIVE.
 */
int PVAttributes::addPV(const char *pName, const char *pDescription, chtype dbrType)
{
    PVAttr *pAttr;
    //const char *functionName = "addPV";

    epicsMutexLock(this->lock);
    pAttr = new PVAttr(pName, pDescription);
    pAttr->dbrType = dbrType;
    pAttr->lock = this->lock;
    ellAdd(&this->PVAttrList, &pAttr->node);
    epicsMutexUnlock(this->lock);
    return (ND_SUCCESS);
}

/** Removes a PV from the PV list, deletes the PVAttr object.
  * \param[in] pName The PV name of the PV. 
 */
int PVAttributes::removePV(const char *pName)
{
    PVAttr *pAttr;
    int status = ND_ERROR;
        
    epicsMutexLock(this->lock);
    pAttr = (PVAttr *)ellFirst(&this->PVAttrList);
    while (pAttr) {
        if (strcmp(pName, pAttr->PVName) == 0) {
            ellDelete(&this->PVAttrList, &pAttr->node);
            delete pAttr;
            status = ND_SUCCESS;
            break;
        }
        pAttr = (PVAttr *)ellNext(&pAttr->node);
    }
    epicsMutexUnlock(this->lock);
    return(status);
}

/** Removes all PVs from the PV list, deletes the PVAttr objects.
 */
int PVAttributes::clearPVs()
{
    PVAttr *pAttr;
    
    epicsMutexLock(this->lock);
    pAttr = (PVAttr *)ellFirst(&this->PVAttrList);
    while (pAttr) {
        ellDelete(&this->PVAttrList, &pAttr->node);
        delete pAttr;
        pAttr = (PVAttr *)ellFirst(&this->PVAttrList);
    }
    epicsMutexUnlock(this->lock);
    return(ND_SUCCESS);
}

/** Returns the current values for all of the EPICS PVs by copying them as attributes
  * to an NDArray object.
  * \param[in] pArray Pointer to an NDArray object. 
   */
int PVAttributes::getValues(NDArray *pArray)
{
    PVAttr *pAttr;
    
    epicsMutexLock(this->lock);
    pAttr = (PVAttr *)ellFirst(&this->PVAttrList);
    while (pAttr) {
        pArray->addAttribute(pAttr->pNDAttribute);
        pAttr = (PVAttr *)ellNext(&pAttr->node);
    }
    epicsMutexUnlock(this->lock);
    return(ND_SUCCESS);
}

/** Reports on the properties of the PVAttributes object; 
  * if details>5 calls PVAttr::report for each PVAttr object in the list.
  */
int PVAttributes::report(int details)
{
    PVAttr *pAttr;
     
    epicsMutexLock(this->lock);
    printf("\n");
    printf("PVAttributes: address=%p:\n", this);
    printf("  number of PVs=%d\n", ellCount(&this->PVAttrList));
    if (details > 5) {
        pAttr = (PVAttr *) ellFirst(&this->PVAttrList);
        while (pAttr) {
            pAttr->report(details);
            pAttr = (PVAttr *) ellNext(&pAttr->node);
        }
    }
    epicsMutexUnlock(this->lock);
    return ND_SUCCESS;
}

