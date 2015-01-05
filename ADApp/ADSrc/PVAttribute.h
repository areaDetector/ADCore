
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

#include <cadef.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include "NDArray.h"

/** Use native type for channel access */
#define DBR_NATIVE -1

/** Attribute that gets its value from an EPICS PV.
  */
class PVAttribute : public NDAttribute {
public:
    PVAttribute(const char *pName, const char *pDescription, const char *pSource, chtype dbrType);
    PVAttribute(PVAttribute& attribute);
    ~PVAttribute();
    PVAttribute* copy(NDAttribute *pAttribute);
    virtual int updateValue();
    /* These callbacks must be public because they are called from C */
    void connectCallback(struct connection_handler_args cha);
    void monitorCallback(struct event_handler_args cha);
    int report(FILE *fp, int details);

private:
    chid        chanId;
    evid        eventId;
    chtype      dbrType;
    NDAttrValue callbackValue;
    char        *callbackString;
    epicsMutexId lock;
};

#endif /*INCPVAttributeH*/
