/** NDAttributeList.cpp
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */
 
#include <stdlib.h>

#include <epicsExport.h>

#include "NDAttributeList.h"

/** NDAttributeList constructor
  */
NDAttributeList::NDAttributeList()
{
  ellInit(&this->list);
  this->lock = epicsMutexCreate();
}

/** NDAttributeList destructor
  */
NDAttributeList::~NDAttributeList()
{
  this->clear();
  ellFree(&this->list);
  epicsMutexDestroy(this->lock);
}

/** Adds an attribute to the list.
  * If an attribute of the same name already exists then
  * the existing attribute is deleted and replaced with the new one.
  * \param[in] pAttribute A pointer to the attribute to add.
  */
int NDAttributeList::add(NDAttribute *pAttribute)
{
  //const char *functionName = "NDAttributeList::add";

  epicsMutexLock(this->lock);
  /* Remove any existing attribute with this name */
  this->remove(pAttribute->pName);
  ellAdd(&this->list, &pAttribute->listNode.node);
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Adds an attribute to the list.
  * This is a convenience function for adding attributes to a list.  
  * It first searches the list to see if there is an existing attribute
  * with the same name.  If there is it just changes the properties of the
  * existing attribute.  If not, it creates a new attribute with the
  * specified properties. 
  * IMPORTANT: This method is only capable of creating attributes
  * of the NDAttribute base class type, not derived class attributes.
  * To add attributes of a derived class to a list the NDAttributeList::add(NDAttribute*)
  * method must be used.
  * \param[in] pName The name of the attribute to be added. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] dataType The data type of the attribute.
  * \param[in] pValue A pointer to the value for this attribute.
  *
  */
NDAttribute* NDAttributeList::add(const char *pName, const char *pDescription, NDAttrDataType_t dataType, void *pValue)
{
  //const char *functionName = "NDAttributeList::add";
  NDAttribute *pAttribute;

  epicsMutexLock(this->lock);
  pAttribute = this->find(pName);
  if (pAttribute) {
    pAttribute->setValue(pValue);
  } else {
    pAttribute = new NDAttribute(pName, pDescription, NDAttrSourceDriver, "Driver", dataType, pValue);
    ellAdd(&this->list, &pAttribute->listNode.node);
  }
  epicsMutexUnlock(this->lock);
  return(pAttribute);
}



/** Finds an attribute by name; the search is now case sensitive (R1-10)
  * \param[in] pName The name of the attribute to be found.
  * \return Returns a pointer to the attribute if found, NULL if not found. 
  */
NDAttribute* NDAttributeList::find(const char *pName)
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::find";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttribute = pListNode->pNDAttribute;
    if (strcmp(pAttribute->pName, pName) == 0) goto done;
    pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
  }
  pAttribute = NULL;

  done:
  epicsMutexUnlock(this->lock);
  return(pAttribute);
}

/** Finds the next attribute in the linked list of attributes.
  * \param[in] pAttributeIn A pointer to the previous attribute in the list; 
  * if NULL the first attribute in the list is returned.
  * \return Returns a pointer to the next attribute if there is one, 
  * NULL if there are no more attributes in the list. */
NDAttribute* NDAttributeList::next(NDAttribute *pAttributeIn)
{
  NDAttribute *pAttribute=NULL;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::next";

  epicsMutexLock(this->lock);
  if (!pAttributeIn) {
    pListNode = (NDAttributeListNode *)ellFirst(&this->list);
   }
  else {
    pListNode = (NDAttributeListNode *)ellNext(&pAttributeIn->listNode.node);
  }
  if (pListNode) pAttribute = pListNode->pNDAttribute;
  epicsMutexUnlock(this->lock);
  return(pAttribute);
}

/** Returns the total number of attributes in the list of attributes.
  * \return Returns the number of attributes. */
int NDAttributeList::count()
{
  //const char *functionName = "NDAttributeList::count";

  return ellCount(&this->list);
}

/** Removes an attribute from the list.
  * \param[in] pName The name of the attribute to be deleted.
  * \return Returns ND_SUCCESS if the attribute was found and deleted, ND_ERROR if the
  * attribute was not found. */
int NDAttributeList::remove(const char *pName)
{
  NDAttribute *pAttribute;
  int status = ND_ERROR;
  //const char *functionName = "NDAttributeList::remove";

  epicsMutexLock(this->lock);
  pAttribute = this->find(pName);
  if (!pAttribute) goto done;
  ellDelete(&this->list, &pAttribute->listNode.node);
  delete pAttribute;
  status = ND_SUCCESS;

  done:
  epicsMutexUnlock(this->lock);
  return(status);
}

/** Deletes all attributes from the list. */
int NDAttributeList::clear()
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::clear";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttribute = pListNode->pNDAttribute;
    ellDelete(&this->list, &pListNode->node);
    delete pAttribute;
    pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  }
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Copies all attributes from one attribute list to another.
  * It is efficient so that if the attribute already exists in the output
  * list it just copies the properties, and memory allocation is minimized.
  * The attributes are added to any existing attributes already present in the output list.
  * \param[out] pListOut A pointer to the output attribute list to copy to.
  */
int NDAttributeList::copy(NDAttributeList *pListOut)
{
  NDAttribute *pAttrIn, *pAttrOut, *pFound;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::copy";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttrIn = pListNode->pNDAttribute;
    /* See if there is already an attribute of this name in the output list */
    pFound = pListOut->find(pAttrIn->pName);
    /* The copy function will copy the properties, and will create the attribute if pFound is NULL */
    pAttrOut = pAttrIn->copy(pFound);
    /* If pFound is NULL, then a copy created a new attribute, need to add it to the list */
    if (!pFound) pListOut->add(pAttrOut);
    pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
  }
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Updates all attribute values in the list; calls NDAttribute::updateValue() for each attribute in the list.
  */
int NDAttributeList::updateValues()
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::updateValues";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttribute = pListNode->pNDAttribute;
    pAttribute->updateValue();
    pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
  }
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Reports on the properties of the attribute list.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; if >10 calls NDAttribute::report() for each attribute.
  */
int NDAttributeList::report(FILE *fp, int details)
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  
  epicsMutexLock(this->lock);
  fprintf(fp, "\n");
  fprintf(fp, "NDAttributeList: address=%p:\n", this);
  fprintf(fp, "  number of attributes=%d\n", this->count());
  if (details > 10) {
    pListNode = (NDAttributeListNode *) ellFirst(&this->list);
    while (pListNode) {
      pAttribute = (NDAttribute *)pListNode->pNDAttribute;
      pAttribute->report(fp, details);
      pListNode = (NDAttributeListNode *) ellNext(&pListNode->node);
    }
  }
  epicsMutexUnlock(this->lock);
  return ND_SUCCESS;
}


