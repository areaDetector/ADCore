/** NDAttribute.cpp
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#include <string>
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
                           
  : dataType_(NDAttrUndefined)
                           
{

  this->name_ = pName ? pName : "";
  this->description_ = pDescription ? pDescription : "";
  this->sourceType_ = sourceType;
  switch (sourceType) {
    case NDAttrSourceDriver:
      this->sourceTypeString_ = "NDAttrSourceDriver";
      break;
    case NDAttrSourceEPICSPV:
      this->sourceTypeString_ = "NDAttrSourceEPICSPV";
      break;
    case NDAttrSourceParam:
      this->sourceTypeString_ = "NDAttrSourceParam";
      break;
    case NDAttrSourceFunct:
      this->sourceTypeString_ = "NDAttrSourceFunct";
      break;
    default:
      this->sourceType_ = NDAttrSourceUndefined;
      this->sourceTypeString_ = "Undefined";
  }
  this->source_ = pSource ? pSource : "";
  this->string_ = "";
  if (pValue) {
    this->setDataType(dataType);
    this->setValue(pValue);
  }
  this->listNode_.pNDAttribute = this;
}

/** NDAttribute copy constructor
  * \param[in] attribute The attribute to copy from
  */
NDAttribute::NDAttribute(NDAttribute& attribute)
{
  void *pValue;
  this->name_ = attribute.name_;
  this->description_ = attribute.description_;
  this->source_ = attribute.source_;
  this->sourceType_ = attribute.sourceType_;
  this->sourceTypeString_ = attribute.sourceTypeString_;
  this->string_ = "";
  this->dataType_ = attribute.dataType_;
  if (attribute.dataType_ == NDAttrString) pValue = (void *)attribute.string_.c_str();
  else pValue = &attribute.value_;
  this->setValue(pValue);
  this->listNode_.pNDAttribute = this;
}


/** NDAttribute destructor */
NDAttribute::~NDAttribute()
{
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
    if (this->dataType_ == NDAttrString) pValue = (void *)this->string_.c_str();
    else pValue = &this->value_;
    pOut->setValue(pValue);
  }
  return pOut;
}

/** Returns the name of this attribute.
  */
const char *NDAttribute::getName()
{
  return name_.c_str();
}

/** Sets the data type of this attribute. This can only be called once.
  */
int NDAttribute::setDataType(NDAttrDataType_t type)
{
  // It is OK to set the data type to the same type as the existing type.
  // This will happen on channel access reconnects and if drivers create a parameter
  // and call this function every time.
  if (type == this->dataType_) return ND_SUCCESS;
  if (this->dataType_ != NDAttrUndefined) {
    fprintf(stderr, "NDAttribute::setDataType, data type already defined = %d\n", this->dataType_);
    return ND_ERROR;
  }
  if ((type < NDAttrInt8) || (type > NDAttrString)) {
    fprintf(stderr, "NDAttribute::setDataType, invalid data type = %d\n", type);
    return ND_ERROR;
  }
  this->dataType_ = type;
  return ND_SUCCESS;
}

/** Returns the data type of this attribute.
  */
NDAttrDataType_t NDAttribute::getDataType()
{
  return dataType_;
}

/** Returns the description of this attribute.
  */
const char *NDAttribute::getDescription()
{
  return description_.c_str();
}

/** Returns the source string of this attribute.
  */
const char *NDAttribute::getSource()
{
  return source_.c_str();
}

/** Returns the source information of this attribute.
  * \param[out] pSourceType Source type (NDAttrSource_t) of this attribute.
  * \return The source type string of this attribute
  */
const char *NDAttribute::getSourceInfo(NDAttrSource_t *pSourceType)
{
  *pSourceType = sourceType_;
  return sourceTypeString_.c_str();
}

/** Sets the value for this attribute. 
  * \param[in] pValue Pointer to the value. */
int NDAttribute::setValue(const void *pValue)
{
  /* If any data type but undefined then pointer must be valid */
  if ((dataType_ != NDAttrUndefined) && !pValue) return ND_ERROR;

  /* Treat strings specially */
  if (dataType_ == NDAttrString) {
    /* If the previous value was the same string don't do anything, 
     * saves freeing and allocating memory.  
     * If not the same free the old string and copy new one. */
    if (this->string_ == (char *)pValue) return ND_SUCCESS;
    this->string_ = (char *)pValue;
    return ND_SUCCESS;
  }
  switch (dataType_) {
    case NDAttrInt8:
      this->value_.i8 = *(epicsInt8 *)pValue;
      break;
    case NDAttrUInt8:
      this->value_.ui8 = *(epicsUInt8 *)pValue;
      break;
    case NDAttrInt16:
      this->value_.i16 = *(epicsInt16 *)pValue;
      break;
    case NDAttrUInt16:
      this->value_.ui16 = *(epicsUInt16 *)pValue;
      break;
    case NDAttrInt32:
      this->value_.i32 = *(epicsInt32*)pValue;
      break;
    case NDAttrUInt32:
      this->value_.ui32 = *(epicsUInt32 *)pValue;
      break;
    case NDAttrFloat32:
      this->value_.f32 = *(epicsFloat32 *)pValue;
      break;
    case NDAttrFloat64:
      this->value_.f64 = *(epicsFloat64 *)pValue;
      break;
    case NDAttrUndefined:
      break;
    default:
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
}

/** Sets the value for this attribute. 
  * \param[in] value value of this attribute. */
int NDAttribute::setValue(const std::string& value)
{
  /* Data type must be string */
  if (dataType_ == NDAttrString) {
    this->string_ = value;
    return ND_SUCCESS;
  }
  return ND_ERROR;
}

/** Returns the data type and size of this attribute.
  * \param[out] pDataType Pointer to location to return the data type.
  * \param[out] pSize Pointer to location to return the data size; this is the
  *  data type size for all data types except NDAttrString, in which case it is the length of the
  * string including 0 terminator. */
int NDAttribute::getValueInfo(NDAttrDataType_t *pDataType, size_t *pSize)
{
  *pDataType = this->dataType_;
  switch (this->dataType_) {
    case NDAttrInt8:
      *pSize = sizeof(this->value_.i8);
      break;
    case NDAttrUInt8:
      *pSize = sizeof(this->value_.ui8);
      break;
    case NDAttrInt16:
      *pSize = sizeof(this->value_.i16);
      break;
    case NDAttrUInt16:
      *pSize = sizeof(this->value_.ui16);
      break;
    case NDAttrInt32:
      *pSize = sizeof(this->value_.i32);
      break;
    case NDAttrUInt32:
      *pSize = sizeof(this->value_.ui32);
      break;
    case NDAttrFloat32:
      *pSize = sizeof(this->value_.f32);
      break;
    case NDAttrFloat64:
      *pSize = sizeof(this->value_.f64);
      break;
    case NDAttrString:
      *pSize = this->string_.size()+1;
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

  switch (this->dataType_) {
    case NDAttrInt8:
      *pValue = (epicsType) this->value_.i8;
      break;
    case NDAttrUInt8:
       *pValue = (epicsType) this->value_.ui8;
      break;
    case NDAttrInt16:
      *pValue = (epicsType) this->value_.i16;
      break;
    case NDAttrUInt16:
      *pValue = (epicsType) this->value_.ui16;
      break;
    case NDAttrInt32:
      *pValue = (epicsType) this->value_.i32;
      break;
    case NDAttrUInt32:
      *pValue = (epicsType) this->value_.ui32;
      break;
    case NDAttrFloat32:
      *pValue = (epicsType) this->value_.f32;
      break;
    case NDAttrFloat64:
      *pValue = (epicsType) this->value_.f64;
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
  switch (this->dataType_) {
    case NDAttrString:
      if (dataType != NDAttrString) return ND_ERROR;
      if (dataSize == 0) dataSize = this->string_.size()+1;
      strncpy((char *)pValue, this->string_.c_str(), dataSize);
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

/** Returns the value of an NDAttrString attribute as an std::string.
  * \param[out] value Location to return the value.
  *
  * Does data type conversions between numeric data types */
int NDAttribute::getValue(std::string& value)
{
  switch (this->dataType_) {
    case NDAttrString:
      value = this->string_;
      return ND_SUCCESS;
    default:
      return ND_ERROR;
  }
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
  fprintf(fp, "  name=%s\n", this->name_.c_str());
  fprintf(fp, "  description=%s\n", this->description_.c_str());
  fprintf(fp, "  source type=%d\n", this->sourceType_);
  fprintf(fp, "  source type string=%s\n", this->sourceTypeString_.c_str());
  fprintf(fp, "  source=%s\n", this->source_.c_str());
  switch (this->dataType_) {
    case NDAttrInt8:
      fprintf(fp, "  dataType=NDAttrInt8\n");
      fprintf(fp, "  value=%d\n", this->value_.i8);
      break;
    case NDAttrUInt8:
      fprintf(fp, "  dataType=NDAttrUInt8\n"); 
      fprintf(fp, "  value=%u\n", this->value_.ui8);
      break;
    case NDAttrInt16:
      fprintf(fp, "  dataType=NDAttrInt16\n"); 
      fprintf(fp, "  value=%d\n", this->value_.i16);
      break;
    case NDAttrUInt16:
      fprintf(fp, "  dataType=NDAttrUInt16\n"); 
      fprintf(fp, "  value=%d\n", this->value_.ui16);
      break;
    case NDAttrInt32:
      fprintf(fp, "  dataType=NDAttrInt32\n"); 
      fprintf(fp, "  value=%d\n", this->value_.i32);
      break;
    case NDAttrUInt32:
      fprintf(fp, "  dataType=NDAttrUInt32\n"); 
      fprintf(fp, "  value=%d\n", this->value_.ui32);
      break;
    case NDAttrFloat32:
      fprintf(fp, "  dataType=NDAttrFloat32\n"); 
      fprintf(fp, "  value=%f\n", this->value_.f32);
      break;
    case NDAttrFloat64:
      fprintf(fp, "  dataType=NDAttrFloat64\n"); 
      fprintf(fp, "  value=%f\n", this->value_.f64);
      break;
    case NDAttrString:
      fprintf(fp, "  dataType=NDAttrString\n"); 
      fprintf(fp, "  value=%s\n", this->string_.c_str());
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


