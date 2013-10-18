/** NDAttribute.cpp
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#include <stdlib.h>

#include <epicsString.h>

#include "NDAttribute.h"

/** NDAttribute constructor
  * \param[in] pName The name of the attribute to be created. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] dataType The data type of the attribute.
  * \param[in] pValue A pointer to the value for this attribute.
  */
NDAttribute::NDAttribute(const char *pName, const char *pDescription, NDAttrDataType_t dataType, void *pValue)
{

  this->pName = epicsStrDup(pName);
  this->pDescription = epicsStrDup(pDescription);
  this->pSource = epicsStrDup("");
  this->sourceType = NDAttrSourceDriver;
  this->pSourceTypeString = epicsStrDup("NDAttrSourceDriver");
  this->pString = NULL;
  if (pValue) this->setValue(dataType, pValue);
  this->listNode.pNDAttribute = this;
}

NDAttribute::NDAttribute(NDAttribute& attribute)
{
  void *pValue;
  this->pName = epicsStrDup(attribute.pName);
  this->pDescription = epicsStrDup(attribute.pDescription);
  this->pSource = epicsStrDup(attribute.pSource);
  this->sourceType = attribute.sourceType;
  this->pSourceTypeString = epicsStrDup(attribute.pSourceTypeString);
  this->pString = NULL;
  this->dataType = attribute.dataType;
  if (attribute.dataType == NDAttrString) pValue = attribute.pString;
  else pValue = &attribute.value;
  this->setValue(attribute.dataType, pValue);
  this->listNode.pNDAttribute = this;
}


/** NDAttribute destructor 
  * Frees the strings for the name, and if they exist, the description and pString. */
NDAttribute::~NDAttribute()
{
  if (this->pName) free(this->pName);
  if (this->pDescription) free(this->pDescription);
  if (this->pSource) free(this->pSource);
  if (this->pString) free(this->pString);
}

/** Copies properties from <b>this</b> to pOut.
  * \param[in] pOut A pointer to the output attribute
  *         If NULL the output attribute will be created.
  * \return  Returns a pointer to the copy
  */
NDAttribute* NDAttribute::copy(NDAttribute *pOut)
{
  void *pValue;
  
  if (!pOut) 
    pOut = new NDAttribute(*this);
  else {
    if (this->dataType == NDAttrString) pValue = this->pString;
    else pValue = &this->value;
    pOut->setValue(this->dataType, pValue);
  }
  return(pOut);
}

/** Sets the description string for this attribute.
  * This method must be used to set the description string; pDescription must not be directly modified. 
  * \param[in] pDescription String with the desciption. */
int NDAttribute::setDescription(const char *pDescription) {

  if (this->pDescription) {
    /* If the new description is the same as the old one return, 
     * saves freeing and allocating memory */
    if (strcmp(this->pDescription, pDescription) == 0) return(ND_SUCCESS);
    free(this->pDescription);
  }
  if (pDescription) this->pDescription = epicsStrDup(pDescription);
  else this->pDescription = NULL;
  return(ND_SUCCESS);
}

/** Sets the source string for this attribute. 
  * This method must be used to set the source string; pSource must not be directly modified. 
  * \param[in] pSource String with the source. */
int NDAttribute::setSource(const char *pSource) {

  if (this->pSource) {
    /* If the new source is the same as the old one return, 
     * saves freeing and allocating memory */
    if (strcmp(this->pSource, pSource) == 0) return(ND_SUCCESS);
    free(this->pSource);
  }
  if (pSource) this->pSource = epicsStrDup(pSource);
  else this->pSource = NULL;
  return(ND_SUCCESS);
}

/** Sets the value for this attribute. 
  * \param[in] dataType Data type of the value.
  * \param[in] pValue Pointer to the value. */
int NDAttribute::setValue(NDAttrDataType_t dataType, void *pValue)
{
  NDAttrDataType_t prevDataType = this->dataType;
    
  this->dataType = dataType;

  /* If any data type but undefined then pointer must be valid */
  if ((dataType != NDAttrUndefined) && !pValue) return(ND_ERROR);

  /* Treat strings specially */
  if (dataType == NDAttrString) {
    /* If the previous value was the same string don't do anything, 
     * saves freeing and allocating memory.  
     * If not the same free the old string and copy new one. */
    if ((prevDataType == NDAttrString) && this->pString) {
      if (strcmp(this->pString, (char *)pValue) == 0) return(ND_SUCCESS);
      free(this->pString);
    }
    this->pString = epicsStrDup((char *)pValue);
    return(ND_SUCCESS);
  }
  if (this->pString) {
    free(this->pString);
    this->pString = NULL;
  }
  switch (dataType) {
    case NDAttrInt8:
      this->value.i8 = *(epicsInt8 *)pValue;
      break;
    case NDAttrUInt8:
      this->value.ui8 = *(epicsUInt8 *)pValue;
      break;
    case NDAttrInt16:
      this->value.i16 = *(epicsInt16 *)pValue;
      break;
    case NDAttrUInt16:
      this->value.ui16 = *(epicsUInt16 *)pValue;
      break;
    case NDAttrInt32:
      this->value.i32 = *(epicsInt32*)pValue;
      break;
    case NDAttrUInt32:
      this->value.ui32 = *(epicsUInt32 *)pValue;
      break;
    case NDAttrFloat32:
      this->value.f32 = *(epicsFloat32 *)pValue;
      break;
    case NDAttrFloat64:
      this->value.f64 = *(epicsFloat64 *)pValue;
      break;
    case NDAttrUndefined:
      break;
    default:
      return(ND_ERROR);
      break;
  }
  return(ND_SUCCESS);
}

/** Returns the data type and size of this attribute.
  * \param[out] pDataType Pointer to location to return the data type.
  * \param[out] pSize Pointer to location to return the data size; this is the
  *  data type size for all data types except NDAttrString, in which case it is the length of the
  * string including 0 terminator. */
int NDAttribute::getValueInfo(NDAttrDataType_t *pDataType, size_t *pSize)
{
  *pDataType = this->dataType;
  switch (this->dataType) {
    case NDAttrInt8:
      *pSize = sizeof(this->value.i8);
      break;
    case NDAttrUInt8:
      *pSize = sizeof(this->value.ui8);
      break;
    case NDAttrInt16:
      *pSize = sizeof(this->value.i16);
      break;
    case NDAttrUInt16:
      *pSize = sizeof(this->value.ui16);
      break;
    case NDAttrInt32:
      *pSize = sizeof(this->value.i32);
      break;
    case NDAttrUInt32:
      *pSize = sizeof(this->value.ui32);
      break;
    case NDAttrFloat32:
      *pSize = sizeof(this->value.f32);
      break;
    case NDAttrFloat64:
      *pSize = sizeof(this->value.f64);
      break;
    case NDAttrString:
      if (this->pString) *pSize = strlen(this->pString)+1;
      else *pSize = 0;
      break;
    case NDAttrUndefined:
      *pSize = 0;
      break;
    default:
      return(ND_ERROR);
      break;
  }
  return(ND_SUCCESS);
}

/** Returns the value of this attribute.
  * \param[in] dataType Data type for the value.
  * \param[out] pValue Pointer to location to return the value.
  * \param[in] dataSize Size of the input data location; only used when dataType is NDAttrString.
  *
  * Currently the dataType parameter is only used to check that it matches the actual data type,
  * and ND_ERROR is returned if it does not.  In the future data type conversion may be added. */
int NDAttribute::getValue(NDAttrDataType_t dataType, void *pValue, size_t dataSize)
{
  if (dataType != this->dataType) return(ND_ERROR);
  switch (this->dataType) {
    case NDAttrInt8:
      *(epicsInt8 *)pValue = this->value.i8;
      break;
    case NDAttrUInt8:
       *(epicsUInt8 *)pValue = this->value.ui8;
      break;
    case NDAttrInt16:
      *(epicsInt16 *)pValue = this->value.i16;
      break;
    case NDAttrUInt16:
      *(epicsUInt16 *)pValue = this->value.ui16;
      break;
    case NDAttrInt32:
      *(epicsInt32*)pValue = this->value.i32;
      break;
    case NDAttrUInt32:
      *(epicsUInt32 *)pValue = this->value.ui32;
      break;
    case NDAttrFloat32:
      *(epicsFloat32 *)pValue = this->value.f32;
      break;
    case NDAttrFloat64:
      *(epicsFloat64 *)pValue = this->value.f64;
      break;
    case NDAttrString:
      if (!this->pString) return (ND_ERROR);
      if (dataSize == 0) dataSize = strlen(this->pString)+1;
      strncpy((char *)pValue, this->pString, dataSize);
      break;
    case NDAttrUndefined:
    default:
      return(ND_ERROR);
      break;
  }
  return ND_SUCCESS ;
}

/** Updates the current value of this attribute.
  * The base class does nothing, but derived classes may fetch the current value of the attribute,
  * for example from an EPICS PV or driver parameter library.
 */
int NDAttribute::updateValue()
{
  return ND_SUCCESS;
}

/** Reports on the properties of the attribute.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; currently does nothing
  */
int NDAttribute::report(FILE *fp, int details)
{
  
  fprintf(fp, "\n");
  fprintf(fp, "NDAttribute, address=%p:\n", this);
  fprintf(fp, "  name=%s\n", this->pName);
  fprintf(fp, "  description=%s\n", this->pDescription);
  fprintf(fp, "  source type=%s\n", this->pSourceTypeString);
  fprintf(fp, "  source=%s\n", this->pSource);
  switch (this->dataType) {
    case NDAttrInt8:
      fprintf(fp, "  dataType=NDAttrInt8\n");
      fprintf(fp, "  value=%d\n", this->value.i8);
      break;
    case NDAttrUInt8:
      fprintf(fp, "  dataType=NDAttrUInt8\n"); 
      fprintf(fp, "  value=%u\n", this->value.ui8);
      break;
    case NDAttrInt16:
      fprintf(fp, "  dataType=NDAttrInt16\n"); 
      fprintf(fp, "  value=%d\n", this->value.i16);
      break;
    case NDAttrUInt16:
      fprintf(fp, "  dataType=NDAttrUInt16\n"); 
      fprintf(fp, "  value=%d\n", this->value.ui16);
      break;
    case NDAttrInt32:
      fprintf(fp, "  dataType=NDAttrInt32\n"); 
      fprintf(fp, "  value=%d\n", this->value.i32);
      break;
    case NDAttrUInt32:
      fprintf(fp, "  dataType=NDAttrUInt32\n"); 
      fprintf(fp, "  value=%d\n", this->value.ui32);
      break;
    case NDAttrFloat32:
      fprintf(fp, "  dataType=NDAttrFloat32\n"); 
      fprintf(fp, "  value=%f\n", this->value.f32);
      break;
    case NDAttrFloat64:
      fprintf(fp, "  dataType=NDAttrFloat64\n"); 
      fprintf(fp, "  value=%f\n", this->value.f64);
      break;
    case NDAttrString:
      fprintf(fp, "  dataType=NDAttrString\n"); 
      fprintf(fp, "  value=%s\n", this->pString);
      break;
    case NDAttrUndefined:
      fprintf(fp, "  dataType=NDAttrUndefined\n");
      break;
    default:
      fprintf(fp, "  dataType=UNKNOWN\n");
      return(ND_ERROR);
      break;
  }
  return ND_SUCCESS;
}


