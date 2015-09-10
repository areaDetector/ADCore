/** NDAttribute.cpp
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#include <stdlib.h>

#include <epicsString.h>

#include <epicsExport.h>

#include "NDAttribute.h"

/** Strings corresponding to the above enums */
static const char *NDAttrSourceStrings[] = {
    "DRIVER",
    "PARAM",
    "EPICS_PV",
    "FUNCTION"
};

const char *NDAttribute::attrSourceString(NDAttrSource_t type)
{
  return NDAttrSourceStrings[type];
}

/** NDAttribute constructor
  * \param[in] pName The name of the attribute to be created. 
  * \param[in] sourceType The source type of the attribute (NDAttrSource_t).
  * \param[in] pSource The source string for the attribute.
  * \param[in] pDescription The description of the attribute.
  * \param[in] dataType The data type of the attribute (NDAttrDataType_t).
  * \param[in] pValue A pointer to the value for this attribute.
  */
  NDAttribute::NDAttribute(const char *pName, const char *pDescription, 
                           NDAttrSource_t sourceType, const char *pSource, 
                           NDAttrDataType_t dataType, void *pValue)
                           
  : dataType(NDAttrUndefined)
                           
{

  this->pName = pName? epicsStrDup(pName): epicsStrDup("");
  this->pDescription = pDescription? epicsStrDup(pDescription): epicsStrDup("");
  this->sourceType = sourceType;
  switch (sourceType) {
    case NDAttrSourceDriver:
      this->pSourceTypeString = epicsStrDup("NDAttrSourceDriver");
      break;
    case NDAttrSourceEPICSPV:
      this->pSourceTypeString = epicsStrDup("NDAttrSourceEPICSPV");
      break;
    case NDAttrSourceParam:
      this->pSourceTypeString = epicsStrDup("NDAttrSourceParam");
      break;
    case NDAttrSourceFunct:
      this->pSourceTypeString = epicsStrDup("NDAttrSourceFunct");
      break;
    default:
      this->sourceType = NDAttrSourceUndefined;
      this->pSourceTypeString = epicsStrDup("Undefined");
  }
  this->pSource = pSource? epicsStrDup(pSource): epicsStrDup("");
  this->pString = NULL;
  if (pValue) {
    this->setDataType(dataType);
    this->setValue(pValue);
  }
  this->listNode.pNDAttribute = this;
}

/** NDAttribute copy constructor
  * \param[in] attribute The attribute to copy from
  */
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
  this->setValue(pValue);
  this->listNode.pNDAttribute = this;
}


/** NDAttribute destructor 
  * Frees the strings for this attribute */
NDAttribute::~NDAttribute()
{
  if (this->pName) free(this->pName);
  if (this->pDescription) free(this->pDescription);
  if (this->pSource) free(this->pSource);
  if (this->pSourceTypeString) free(this->pSourceTypeString);
  if (this->pString) free(this->pString);
}

/** Copies properties from <b>this</b> to pOut.
  * \param[in] pOut A pointer to the output attribute
  *         If NULL the output attribute will be created using the copy constructor
  * Only the value is copied, all other fields are assumed to already be the same in pOut
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
    pOut->setValue(pValue);
  }
  return pOut;
}

/** Returns the name of this attribute.
  */
const char *NDAttribute::getName()
{
  return pName;
}

/** Sets the data type of this attribute. This can only be called once.
  */
int NDAttribute::setDataType(NDAttrDataType_t type)
{
  // It is OK to set the data type to the same type as the existing type.
  // This will happen on channel access reconnects and if drivers create a parameter
  // and call this function every time.
  if (type == this->dataType) return ND_SUCCESS;
  if (this->dataType != NDAttrUndefined) {
    fprintf(stderr, "NDAttribute::setDataType, data type already defined = %d\n", this->dataType);
    return ND_ERROR;
  }
  if ((type < NDAttrInt8) || (type > NDAttrString)) {
    fprintf(stderr, "NDAttribute::setDataType, invalid data type = %d\n", type);
    return ND_ERROR;
  }
  this->dataType = type;
  return ND_SUCCESS;
}

/** Returns the data type of this attribute.
  */
NDAttrDataType_t NDAttribute::getDataType()
{
  return dataType;
}

/** Returns the description of this attribute.
  */
const char *NDAttribute::getDescription()
{
  return pDescription;
}

/** Returns the source string of this attribute.
  */
const char *NDAttribute::getSource()
{
  return pSource;
}

/** Returns the source information of this attribute.
  * \param[out] pSourceType Source type (NDAttrSource_t) of this attribute.
  * \return The source type string of this attribute
  */
const char *NDAttribute::getSourceInfo(NDAttrSource_t *pSourceType)
{
  *pSourceType = sourceType;
  return pSourceTypeString;
}

/** Sets the value for this attribute. 
  * \param[in] pValue Pointer to the value. */
int NDAttribute::setValue(void *pValue)
{
  /* If any data type but undefined then pointer must be valid */
  if ((dataType != NDAttrUndefined) && !pValue) return ND_ERROR;

  /* Treat strings specially */
  if (dataType == NDAttrString) {
    /* If the previous value was the same string don't do anything, 
     * saves freeing and allocating memory.  
     * If not the same free the old string and copy new one. */
    if (this->pString) {
      if (strcmp(this->pString, (char *)pValue) == 0) return ND_SUCCESS;
      free(this->pString);
    }
    this->pString = epicsStrDup((char *)pValue);
    return ND_SUCCESS;
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
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
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
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
}

template <typename epicsType>
int NDAttribute::getValueT(void *pValueIn, size_t dataSize)
{
  epicsType *pValue = (epicsType *)pValueIn;

  switch (this->dataType) {
    case NDAttrInt8:
      *pValue = (epicsType) this->value.i8;
      break;
    case NDAttrUInt8:
       *pValue = (epicsType) this->value.ui8;
      break;
    case NDAttrInt16:
      *pValue = (epicsType) this->value.i16;
      break;
    case NDAttrUInt16:
      *pValue = (epicsType) this->value.ui16;
      break;
    case NDAttrInt32:
      *pValue = (epicsType) this->value.i32;
      break;
    case NDAttrUInt32:
      *pValue = (epicsType) this->value.ui32;
      break;
    case NDAttrFloat32:
      *pValue = (epicsType) this->value.f32;
      break;
    case NDAttrFloat64:
      *pValue = (epicsType) this->value.f64;
      break;
    default:
      return ND_ERROR;
  }
  return ND_SUCCESS ;
}


/** Returns the value of this attribute.
  * \param[in] dataType Data type for the value.
  * \param[out] pValue Pointer to location to return the value.
  * \param[in] dataSize Size of the input data location; only used when dataType is NDAttrString.
  *
  * Does data type conversions between numeric data types */
int NDAttribute::getValue(NDAttrDataType_t dataType, void *pValue, size_t dataSize)
{
  switch (this->dataType) {
    case NDAttrString:
      if (dataType != NDAttrString) return ND_ERROR;
      if (!this->pString) return ND_ERROR;
      if (dataSize == 0) dataSize = strlen(this->pString)+1;
      strncpy((char *)pValue, this->pString, dataSize);
      return ND_SUCCESS;
    case NDAttrUndefined:
      return ND_ERROR;
    default:
      break;
  }
  
  switch (dataType) {
    case NDAttrInt8:
      return getValueT<epicsInt8>(pValue, dataSize);
    case NDAttrUInt8:
      return getValueT<epicsUInt8>(pValue, dataSize);
    case NDAttrInt16:
      return getValueT<epicsInt16>(pValue, dataSize);
      break;
    case NDAttrUInt16:
      return getValueT<epicsUInt16>(pValue, dataSize);
    case NDAttrInt32:
      return getValueT<epicsInt32>(pValue, dataSize);
    case NDAttrUInt32:
      return getValueT<epicsUInt32>(pValue, dataSize);
    case NDAttrFloat32:
      return getValueT<epicsFloat32>(pValue, dataSize);
    case NDAttrFloat64:
      return getValueT<epicsFloat64>(pValue, dataSize);
    default:
      return ND_ERROR;
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
  fprintf(fp, "  source type=%d\n", this->sourceType);
  fprintf(fp, "  source type string=%s\n", this->pSourceTypeString);
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
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
}


