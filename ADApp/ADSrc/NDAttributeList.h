/** NDAttributeList.h
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#ifndef NDAttributeList_H
#define NDAttributeList_H

#include <stdio.h>
#include <ellLib.h>
#include <epicsMutex.h>
 
#include "NDAttribute.h"


/** NDAttributeList class; this is a linked list of attributes.
  */
class epicsShareClass NDAttributeList {
public:
    NDAttributeList();
    ~NDAttributeList();
    int          add(NDAttribute *pAttribute);
    NDAttribute* add(const char *pName, const char *pDescription="", 
                     NDAttrDataType_t dataType=NDAttrUndefined, void *pValue=NULL);
    NDAttribute* find(const char *pName);
    NDAttribute* next(NDAttribute *pAttribute);
    int          count();
    int          remove(const char *pName);
    int          clear();
    int          copy(NDAttributeList *pOut);
    int          updateValues();
    int          report(FILE *fp, int details);
    
private:
    ELLLIST      list;   /**< The EPICS ELLLIST  */
    epicsMutexId lock;  /**< Mutex to protect the ELLLIST */
};

#endif

