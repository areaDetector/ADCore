/* PVAttributes.h
 *
 * \author Mark Rivers
 *
 * \author University of Chicago
 *
 * \date April 30, 2009
 *
 */
#ifndef INCPVAttributesH
#define INCPVAttributesH

#include <ellLib.h>
#include <cadef.h>

#include "NDArray.h"

/** Use native type for channel access */
#define DBR_NATIVE -1

/** Maintains the current values of a set of EPICS PVs.  
  * Puts value monitors on the PVs when they connect so that the values
  * are always current.
  * The values are stored in a link list which includes an NDAttribute
  * object for each PV.  The NDAttribute name is the EPICS PV name.
  * The NDAttribute description is set with PVAttributes::addPV.
  * The values are retrieve by calling getValues, which will add the
  * attributes to the NDArray object that is passed in.
  */
class PVAttributes {
public:
    /* Methods */
    PVAttributes();
    ~PVAttributes();
    int addPV(const char *pName, const char *pDescription, chtype dbrType);
    int removePV(const char *pName);
    int clearPVs();
    int getValues(NDArray *pArray);
    int report(int details);

private:
    ELLLIST      PVAttrList;      /**< Linked list of PVAttr  */
    epicsMutexId lock;            /**< Mutex to protect the free list */
    ca_client_context *pCaInputContext;
    
};


/** Maintains the current value for a single EPICS PV.
  * Used by the PVAttributes class, not really intended for use
  * outside that class.
  */
class PVAttr {
public:
    PVAttr(const char *pName, const char *pDescription);
    ~PVAttr();
    /* These callbacks must be public because they are called from C */
    void connectCallback(struct connection_handler_args cha);
    void monitorCallback(struct event_handler_args cha);
    int report(int details);
private:
    
    ELLNODE node;   /**< This must come first because ELLNODE must have the same address as PVAttr object */
	NDAttribute *pNDAttribute;
    chid		chanId;
    evid		eventId;
    char        *PVName;
    chtype      dbrType;
    NDAttrDataType_t dataType;
    epicsMutexId lock;
    
    friend class PVAttributes;
};


#endif /*INCPVAttributesH*/
