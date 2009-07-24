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

#include <asynDriver.h>

#include "NDArray.h"
#include "PVAttribute.h"
#include "tinyxml.h"

#define BUFFER_SIZE 256

static const char *driverName = "PVAttribute";

/* This asynUser is not attached to any device.  
 * It lets one turn on debugging by settings the global asynTrace flag bits
 * ASTN_TRACE_ERROR (default), ASYN_TRACE_FLOW, etc. */

static asynUser *pasynUserSelf = NULL;

static void monitorCallbackC(struct event_handler_args cha)
{
    PVAttribute *pAttribute = (PVAttribute *)ca_puser(cha.chid);
    if (!pAttribute) return;
    pAttribute->monitorCallback(cha);
}

/** Monitor callback called whenever an EPICS PV changes value.
  * Calls NDAttribute::setValue to store the new value.
  */
void PVAttribute::monitorCallback(struct event_handler_args eha)
{
    //chid  chanId = eha.chid;
    const char *functionName = "monitorCallback";

    epicsMutexLock(this->lock);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s:%s: PV=%s\n", 
        driverName, functionName, this->pSource);

    if (eha.status != ECA_NORMAL) {
        asynPrint(pasynUserSelf,  ASYN_TRACE_ERROR,
        "%s:%s: CA returns eha.status=%d\n",
        driverName, functionName, eha.status);
        goto done;
    }
    this->setValue(this->dataType, (void *)eha.dbr);
    done:
    epicsMutexUnlock(this->lock);
}


static void connectCallbackC(struct connection_handler_args cha)
{
    PVAttribute    *pAttribute = (PVAttribute *)ca_puser(cha.chid);
    if (!pAttribute) return;
    pAttribute->connectCallback(cha);
}

/** Connection callback called whenever an EPICS PV connects or disconnects.
  * If it is a connection event it calls ca_add_masked_array_event to request
  *  callbacks whenever the value changes.
  */
void PVAttribute::connectCallback(struct connection_handler_args cha)
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
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Connect event, PV=%s, chanId=%d, dbfType=%d, elementCount=%d, dbrType=%d\n", 
            driverName, functionName, this->pSource, chanId, dbfType, elementCount, dbrType);
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
            driverName, functionName, this->pSource, chanId, this->dataType);
            
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
        this->setValue(NDAttrUndefined, 0);
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s:%s: Disconnect event, PV=%s, chanId=%d\n", 
            driverName, functionName, this->pSource, chanId);
    }
    done:
    epicsMutexUnlock(this->lock);
}

/** Reports on the properties of the PVAttribute object; 
  * calls NDAttribute::report to report on the EPICS PV value.
  */
int PVAttribute::report(int details)
{
    epicsMutexLock(this->lock);
    printf("PVAttribute, address=%p:\n", this);
    printf("  Source type=%d\n", this->sourceType);
    printf("  Source=%s\n", this->pSource);
    printf("  dbrType=%s\n", dbr_type_to_text(this->dbrType));
    NDAttribute::report(details);
    epicsMutexUnlock(this->lock);
    return(ND_SUCCESS);
}
    
/** PVAttribute constructor
  */
PVAttribute::PVAttribute(const char *pName, const char *pDescription,
                         const char *pSource, chtype dbrType)
    : NDAttribute(pName)
{
    /* Create the static pasynUser if not already done */
    if (!pasynUserSelf) pasynUserSelf = pasynManager->createAsynUser(0,0);
    this->lock = epicsMutexCreate();
    if (pDescription) this->setDescription(pDescription);
    if (pSource) this->setSource(pSource);
    this->dbrType = dbrType;
    this->sourceType = NDAttrSourceEPICSPV;
    /* Set connection callback on this PV */
    SEVCHK(ca_create_channel(pSource, connectCallbackC, this, 10 ,&this->chanId),
           "ca_create_channel");
    
}

PVAttribute::~PVAttribute()
{
    if (this->chanId) SEVCHK(ca_clear_channel(this->chanId),"ca_clear_channel");
    epicsMutexDestroy(this->lock);
}


/** PVAttributeList constructor
  */
PVAttributeList::PVAttributeList()
{
    /* Create the static pasynUser if not already done */
    if (!pasynUserSelf) pasynUserSelf = pasynManager->createAsynUser(0,0);
    ellInit(&this->PVAttrList);
    this->lock = epicsMutexCreate();
    SEVCHK(ca_context_create(ca_enable_preemptive_callback),"ca_context_create");
    this->pCaInputContext = NULL;
    while (this->pCaInputContext == NULL) {
        epicsThreadSleep(epicsThreadSleepQuantum());
        this->pCaInputContext = ca_current_context();
    }
}

/** PVAttributeList destructor
  */
PVAttributeList::~PVAttributeList()
{
    this->clearAttributes();
    ellFree(&this->PVAttrList);
    epicsMutexDestroy(this->lock);
}

/** Adds a new PVAttribute to the attribute list, puts connect and monitor callbacks on the PV.
  * \param[in] pName The PV name of the PV. 
  * \param[in] pDescription Description of the PV.
  * \param[in] dbrType DBR_XXX type or DBR_NATIVE.
 */
int PVAttributeList::addPV(const char *pName, const char *pDescription, const char *pSource, chtype dbrType)
{
    PVAttribute *pAttribute;
    //const char *functionName = "addPV";

    epicsMutexLock(this->lock);
    /* Need to attach to the ca_context because this method could be called in a different thread from
     * that which called the constructor */
    ca_attach_context(this->pCaInputContext);
    pAttribute = new PVAttribute(pName, pDescription, pSource, dbrType);
    ellAdd(&this->PVAttrList, &pAttribute->listNode.node);
    epicsMutexUnlock(this->lock);
    return (ND_SUCCESS);
}

/** Removes an attribute from list, deletes the PVAttribute object.
  * \param[in] pName The name of the attribute. 
 */
int PVAttributeList::removeAttribute(const char *pName)
{
    PVAttribute *pAttribute;
    NDAttributeListNode *pListNode;
    int status = ND_ERROR;
        
    epicsMutexLock(this->lock);
    pListNode = (NDAttributeListNode *)ellFirst(&this->PVAttrList);
    while (pListNode) {
        pAttribute = (PVAttribute *)pListNode->pNDAttribute;
        if (strcmp(pName, pAttribute->pName) == 0) {
            ellDelete(&this->PVAttrList, &pListNode->node);
            delete pAttribute;
            status = ND_SUCCESS;
            break;
        }
        pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
    }
    epicsMutexUnlock(this->lock);
    return(status);
}

/** Removes allattributes from the attribute list, deletes the PVAttribute objects.
 */
int PVAttributeList::clearAttributes()
{
    PVAttribute *pAttribute;
    NDAttributeListNode *pListNode;
    
    epicsMutexLock(this->lock);
    pListNode = (NDAttributeListNode *)ellFirst(&this->PVAttrList);
    while (pListNode) {
        pAttribute = (PVAttribute *)pListNode->pNDAttribute;
        ellDelete(&this->PVAttrList, &pListNode->node);
        delete pAttribute;
        pListNode = (NDAttributeListNode *)ellFirst(&this->PVAttrList);
    }
    epicsMutexUnlock(this->lock);
    return(ND_SUCCESS);
}

int PVAttributeList::readFile(const char *fileName)
{
    const char *functionName = "readAttributesFile";
    const char *pName, *pDBRType, *pSource, *pType, *pDescription=NULL;
    int dbrType;
    TiXmlDocument doc(fileName);
    TiXmlElement *Attr, *Attrs;
    
    if (!doc.LoadFile()) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: cannot open file %s error=%s\n", 
            driverName, functionName, fileName, doc.ErrorDesc());
        return(asynError);
    }
    Attrs = doc.FirstChildElement( "Attributes" );
    if (!Attrs) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: cannot find Attributes element\n", 
            driverName, functionName);
        return(asynError);
    }
    for (Attr = Attrs->FirstChildElement(); Attr; Attr = Attr->NextSiblingElement()) {
        pName = Attr->Attribute("name");
        if (!pName) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: name attribute not found\n", 
                driverName, functionName);
            return(asynError);
        }
        pDescription = Attr->Attribute("description");
        pSource = Attr->Attribute("source");
        pType = Attr->Attribute("type");
        if (!pType) pType = "EPICS_PV";
        if (strcmp(pType, "EPICS_PV") == 0) {
            pDBRType = Attr->Attribute("dbrtype");
            dbrType = DBR_NATIVE;
            if (pDBRType) {
                if      (!strcmp(pDBRType, "DBR_CHAR"))   dbrType = DBR_CHAR;
                else if (!strcmp(pDBRType, "DBR_SHORT"))  dbrType = DBR_SHORT;
                else if (!strcmp(pDBRType, "DBR_ENUM"))   dbrType = DBR_ENUM;
                else if (!strcmp(pDBRType, "DBR_INT"))    dbrType = DBR_INT;
                else if (!strcmp(pDBRType, "DBR_LONG"))   dbrType = DBR_LONG;
                else if (!strcmp(pDBRType, "DBR_FLOAT"))  dbrType = DBR_FLOAT;
                else if (!strcmp(pDBRType, "DBR_DOUBLE")) dbrType = DBR_DOUBLE;
                else if (!strcmp(pDBRType, "DBR_STRING")) dbrType = DBR_STRING;
                else if (!strcmp(pDBRType, "DBR_NATIVE")) dbrType = DBR_NATIVE;
                else {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: unknown dbrType = %s\n", 
                        driverName, functionName, pDBRType);
                    return(asynError);
                }
            }
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s: Name=%s, PVName=%s, pDBRType=%s, dbrType=%d, pDescription=%s\n",
                driverName, functionName, pName, pSource, pDBRType, dbrType, pDescription);
            this->addPV(pName, pDescription, pSource, dbrType);
        }
    }
    return(asynSuccess);
}


/** Returns the current values for all of the EPICS PVs by copying them as attributes
  * to an NDArray object.
  * \param[in] pArray Pointer to an NDArray object. 
   */
int PVAttributeList::getValues(NDArray *pArray)
{
    PVAttribute *pAttribute;
    NDAttributeListNode *pListNode;
    
    epicsMutexLock(this->lock);
    pListNode = (NDAttributeListNode *)ellFirst(&this->PVAttrList);
    while (pListNode) {
        pAttribute = (PVAttribute *)pListNode->pNDAttribute;
        pArray->addAttribute(pAttribute);
        pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
    }
    epicsMutexUnlock(this->lock);
    return(ND_SUCCESS);
}

/** Returns the number of attributes in the list.
   */
int PVAttributeList::numAttributes()
{
    return(ellCount(&this->PVAttrList));
}

/** Reports on the properties of the PVAttributeList object; 
  * if details>5 calls PVAttribute::report for each PVAttribute object in the list.
  */
int PVAttributeList::report(int details)
{
    PVAttribute *pAttribute;
    NDAttributeListNode *pListNode;
     
    epicsMutexLock(this->lock);
    printf("\n");
    printf("PVAttributeList: address=%p:\n", this);
    printf("  number of attributes=%d\n", this->numAttributes());
    if (details > 5) {
        pListNode = (NDAttributeListNode *) ellFirst(&this->PVAttrList);
        while (pListNode) {
            pAttribute = (PVAttribute *)pListNode->pNDAttribute;
            pAttribute->report(details);
            pListNode = (NDAttributeListNode *) ellNext(&pListNode->node);
        }
    }
    epicsMutexUnlock(this->lock);
    return ND_SUCCESS;
}

