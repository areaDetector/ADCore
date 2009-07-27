
/* PVAttribute.h
 *
 * \author Mark Rivers
 *
 * \author University of Chicago
 *
 * \date April 30, 2009
 *
 */
#ifndef INCPVAttributeH
#define INCPVAttributeH

#include <ellLib.h>
#include <cadef.h>

#include "NDArray.h"
#include "asynNDArrayDriver.h"

/** Use native type for channel access */
#define DBR_NATIVE -1

typedef enum {
    PVAttrParamInt,
    PVAttrParamDouble,
    PVAttrParamString,
    PVAttrParamUnknown
} PVAttrParamType;

/** Maintains the current value for a single EPICS PV.
  * Used by the PVAttributeList class, not really intended for use
  * outside that class.
  */
class PVAttribute : public NDAttribute {
public:
    PVAttribute(const char *pName, const char *pDescription, const char *pSource, chtype dbrType);
    PVAttribute(const char *pName, const char *pDescription, const char *pSource, int addr, 
                    class asynNDArrayDriver *pDriver, const char *dataType);
    ~PVAttribute();
    /* These callbacks must be public because they are called from C */
    void connectCallback(struct connection_handler_args cha);
    void monitorCallback(struct event_handler_args cha);
    int report(int details);

private:
    chid        chanId;
    evid        eventId;
    chtype      dbrType;
    int         paramId;
    int         paramAddr;
    PVAttrParamType paramType;
    class asynNDArrayDriver *pDriver;
    epicsMutexId lock;
    
    friend class PVAttributeList;
};

/** Maintains the current values of a set of attributes.  These values can come from EPICS PVs
  * or from asynPortDriver parameter library. 
  * Puts value monitors on the PVs when they connect so that the values
  * are always current.
  * The values are retrieved by calling getValues, which will add the
  * attributes to the NDArray object that is passed in.
  */
class PVAttributeList {
public:
    /* Methods */
    PVAttributeList(class asynNDArrayDriver *pDriver);
    ~PVAttributeList();
    int addPV(const char *pName, const char *pDescription, const char *pSource, chtype dbrType);
    int addParam(const char *pName, const char *pDescription, const char *pSource, 
                class asynNDArrayDriver *pDriver, int addr, const char *dataType);
    int removeAttribute(const char *pName);
    int clearAttributes();
    int readFile(const char *fileName);
    int getValues(NDArray *pArray);
    int numAttributes();
    int report(int details);
    class asynNDArrayDriver *pDriver;

private:
    ELLLIST      PVAttrList; /**< Linked list of PVAttributes  */
    epicsMutexId lock;            /**< Mutex to protect the free list */
    ca_client_context *pCaInputContext;
    
};


#endif /*INCPVAttributeH*/
