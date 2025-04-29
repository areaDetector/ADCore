#include "ntndArrayConverter.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;

template <typename dataType>
struct freeNDArray {
    NDArray *array;
    freeNDArray(NDArray *array) : array(array) {};
    void operator()(dataType *data) { array->release(); }
};

NTNDArrayConverter::NTNDArrayConverter (pvxs::Value value) : m_value(value) {}

NDColorMode_t NTNDArrayConverter::getColorMode (void)
{
    auto attributes = m_value["attribute"].as<pvxs::shared_array<const pvxs::Value>>();
    NDColorMode_t colorMode = NDColorMode_t::NDColorModeMono;
    for (int i=0; i<attributes.size(); i++) {
        pvxs::Value attribute = attributes[i];
        if (attribute["name"].as<std::string>() == "ColorMode") {
            colorMode = (NDColorMode_t) attribute["value"].as<int32_t>();
            break;
        }
    }
    return colorMode;
}

NTNDArrayInfo_t NTNDArrayConverter::getInfo (void)
{
    NTNDArrayInfo_t info = {0};

    auto dims = m_value["dimension"].as<pvxs::shared_array<const pvxs::Value>>();
    info.ndims = (int) dims.size();
    info.nElements = 1;

    for(int i = 0; i < info.ndims; ++i)
    {
        info.dims[i]    = dims[i]["size"].as<size_t>();
        info.nElements *= info.dims[i];
    }

    info.codec = m_value["codec.name"].as<std::string>();

    NDDataType_t dt;
    int bpe;

    if (info.codec.empty()) {
        // TODO would be nicer as a switch statement
        std::string fieldName = m_value["value"].nameOf(m_value["value"]["->"]);
        std::string typeName(fieldName.substr(0, fieldName.find("Value")));
        if (typeName == "byte")         {dt = NDDataType_t::NDInt8;      bpe = sizeof(int8_t);}
        else if (typeName == "ubyte")   {dt = NDDataType_t::NDUInt8;     bpe = sizeof(uint8_t);}
        else if (typeName == "short")   {dt = NDDataType_t::NDInt16;     bpe = sizeof(int16_t);}
        else if (typeName == "ushort")  {dt = NDDataType_t::NDUInt16;    bpe = sizeof(uint16_t);}
        else if (typeName == "int")     {dt = NDDataType_t::NDInt32;     bpe = sizeof(int32_t);}
        else if (typeName == "uint")    {dt = NDDataType_t::NDUInt32;    bpe = sizeof(uint32_t);}
        else if (typeName == "long")    {dt = NDDataType_t::NDInt64;     bpe = sizeof(int64_t);}
        else if (typeName == "ulong")   {dt = NDDataType_t::NDUInt64;    bpe = sizeof(uint64_t);}
        else if (typeName == "float")   {dt = NDDataType_t::NDFloat32;   bpe = sizeof(float_t);}
        else if (typeName == "double")  {dt = NDDataType_t::NDFloat64;   bpe = sizeof(double_t);}
        else throw std::runtime_error("invalid value data type");
        // TODO get datatype
    } else {
        throw std::runtime_error("Have not implemeted parsing from known codec type yet");
        // get datatype from codec.parameters...
    }

    info.dataType        = dt;
    info.bytesPerElement = bpe;
    info.totalBytes      = info.nElements*info.bytesPerElement;
    info.colorMode       = getColorMode();

    if(info.ndims > 0)
    {
        info.x.dim    = 0;
        info.x.stride = 1;
        info.x.size   = info.dims[0];
    }

    if(info.ndims > 1)
    {
        info.y.dim    = 1;
        info.y.stride = 1;
        info.y.size   = info.dims[1];
    }

    if(info.ndims == 3)
    {
        switch(info.colorMode)
        {
        case NDColorModeRGB1:
            info.x.dim        = 1;
            info.y.dim        = 2;
            info.color.dim    = 0;
            info.x.stride     = info.dims[0];
            info.y.stride     = info.dims[0]*info.dims[1];
            info.color.stride = 1;
            break;

        case NDColorModeRGB2:
            info.x.dim        = 0;
            info.y.dim        = 2;
            info.color.dim    = 1;
            info.x.stride     = 1;
            info.y.stride     = info.dims[0]*info.dims[1];
            info.color.stride = info.dims[0];
            break;

        case NDColorModeRGB3:
            info.x.dim        = 1;
            info.y.dim        = 2;
            info.color.dim    = 0;
            info.x.stride     = info.dims[0];
            info.y.stride     = info.dims[0]*info.dims[1];
            info.color.stride = 1;
            break;

        default:
            info.x.dim        = 0;
            info.y.dim        = 1;
            info.color.dim    = 2;
            info.x.stride     = 1;
            info.y.stride     = info.dims[0];
            info.color.stride = info.dims[0]*info.dims[1];
            break;
        }

        info.x.size     = info.dims[info.x.dim];
        info.y.size     = info.dims[info.y.dim];
        info.color.size = info.dims[info.color.dim];
    }

    return info;
}

void NTNDArrayConverter::toArray (NDArray *dest)
{
    toValue(dest);
    toDimensions(dest);
    toTimeStamp(dest);
    toDataTimeStamp(dest);
    toAttributes(dest);

    dest->uniqueId = m_value["uniqueId"].as<int32_t>();
}

void NTNDArrayConverter::fromArray (NDArray *src)
{
    fromValue(src);
    fromDimensions(src);
    fromTimeStamp(src);
    fromDataTimeStamp(src);
    fromAttributes(src);

    m_value["uniqueId"] = src->uniqueId;
}

template <typename arrayType>
void NTNDArrayConverter::toValue (NDArray *dest, std::string fieldName)
{
    NTNDArrayInfo_t info = getInfo();
    dest->codec.name = info.codec;
    dest->dataType = info.dataType;

    auto value = m_value[fieldName].as<pvxs::shared_array<const arrayType>>();
    memcpy(dest->pData, value.data(), info.totalBytes);

    if (!info.codec.empty())
        dest->compressedSize = info.totalBytes;
}

void NTNDArrayConverter::toValue (NDArray *dest)
{
    std::string fieldName = m_value["value"].nameOf(m_value["value"]["->"]);
    std::string typeName(fieldName.substr(0, fieldName.find("Value")));
    if (typeName == "byte")         {toValue<int8_t>(dest, std::string("value->byteValue"));}
    else if (typeName == "ubyte")   {toValue<uint8_t>(dest, std::string("value->ubyteValue"));}
    else if (typeName == "short")   {toValue<int16_t>(dest, std::string("value->shortValue"));}
    else if (typeName == "ushort")  {toValue<uint16_t>(dest, std::string("value->ushortValue"));}
    else if (typeName == "int")     {toValue<int32_t>(dest, std::string("value->intValue"));}
    else if (typeName == "uint")    {toValue<uint32_t>(dest, std::string("value->uintValue"));}
    else if (typeName == "long")    {toValue<int64_t>(dest, std::string("value->longValue"));}
    else if (typeName == "ulong")   {toValue<uint64_t>(dest, std::string("value->ulongValue"));}
    else if (typeName == "float")   {toValue<float_t>(dest, std::string("value->floatValue"));}
    else if (typeName == "double")  {toValue<double_t>(dest, std::string("value->doubleValue"));}
    else throw std::runtime_error("invalid value data type");
}

void NTNDArrayConverter::toDimensions (NDArray *dest)
{
    auto dims = m_value["dimension"].as<pvxs::shared_array<const pvxs::Value>>();
    dest->ndims = (int)dims.size();

    for(size_t i = 0; i < dest->ndims; ++i)
    {
        NDDimension_t *d = &dest->dims[i];
        d->size    = dims[i]["size"].as<int32_t>();
        d->offset  = dims[i]["offset"].as<int32_t>();
        d->binning = dims[i]["binning"].as<int32_t>();
        d->reverse = dims[i]["reverse"].as<bool>();
    }
}

void NTNDArrayConverter::toTimeStamp (NDArray *dest)
{
    // NDArray uses EPICS time, pvAccess uses Posix time, need to convert
    dest->epicsTS.secPastEpoch = (epicsUInt32)
        m_value["timeStamp.secondsPastEpoch"].as<uint32_t>() - POSIX_TIME_AT_EPICS_EPOCH;
    dest->epicsTS.nsec = (epicsUInt32)
        m_value["timeStamp.nanoseconds"].as<uint32_t>();
}

void NTNDArrayConverter::toDataTimeStamp (NDArray *dest)
{
    // NDArray uses EPICS time, pvAccess uses Posix time, need to convert
    dest->timeStamp = (epicsFloat64) 
        (m_value["dataTimeStamp.nanoseconds"].as<double_t>() / 1e9) 
        + m_value["dataTimeStamp.secondsPastEpoch"].as<uint32_t>() 
        - POSIX_TIME_AT_EPICS_EPOCH;
}

template <typename valueType>
void NTNDArrayConverter::toAttribute (NDArray *dest, pvxs::Value attribute, NDAttrDataType_t dataType)
{
    // TODO, can we make dataType a template parameter?
    auto name = attribute["name"].as<std::string>();
    auto desc = attribute["descriptor"].as<std::string>();
    auto source = attribute["source"].as<std::string>();
    NDAttrSource_t sourceType = (NDAttrSource_t) attribute["sourceType"].as<int32_t>();
    valueType value = attribute["value"].as<valueType>();

    NDAttribute *attr = new NDAttribute(name.c_str(), desc.c_str(), sourceType, source.c_str(), dataType, (void*)&value);
    dest->pAttributeList->add(attr);
}

void NTNDArrayConverter::toStringAttribute (NDArray *dest, pvxs::Value attribute)
{
    auto name = attribute["name"].as<std::string>();
    auto desc = attribute["descriptor"].as<std::string>();
    auto source = attribute["source"].as<std::string>();
    NDAttrSource_t sourceType = (NDAttrSource_t) attribute["sourceType"].as<int32_t>();
    auto value = attribute["value"].as<std::string>();

    NDAttribute *attr = new NDAttribute(name.c_str(), desc.c_str(), sourceType, source.c_str(), NDAttrDataType_t::NDAttrString, (void*)value.c_str());
    dest->pAttributeList->add(attr);
}

void NTNDArrayConverter::toUndefinedAttribute (NDArray *dest, pvxs::Value attribute)
{
    auto name = attribute["name"].as<std::string>();
    auto desc = attribute["descriptor"].as<std::string>();
    auto source = attribute["source"].as<std::string>();
    NDAttrSource_t sourceType = (NDAttrSource_t) attribute["sourceType"].as<int32_t>();

    NDAttribute *attr = new NDAttribute(name.c_str(), desc.c_str(), sourceType, source.c_str(), NDAttrDataType_t::NDAttrUndefined, NULL);
    dest->pAttributeList->add(attr);
}

void NTNDArrayConverter::toAttributes (NDArray *dest)
{
    auto attributes = m_value["attribute"].as<pvxs::shared_array<const pvxs::Value>>();
    for (int i=0; i<attributes.size(); i++) {
        pvxs::Value value = attributes[i]["value"];
        switch (attributes[i]["value->"].type().code) {
            // use indirection on Any container to get specified type
            case pvxs::TypeCode::Int8:        toAttribute<int8_t>    (dest, attributes[i], NDAttrDataType_t::NDAttrInt8); break;
            case pvxs::TypeCode::UInt8:       toAttribute<uint8_t>   (dest, attributes[i], NDAttrDataType_t::NDAttrUInt8); break;
            case pvxs::TypeCode::Int16:       toAttribute<int16_t>   (dest, attributes[i], NDAttrDataType_t::NDAttrInt16); break;
            case pvxs::TypeCode::UInt16:      toAttribute<uint16_t>  (dest, attributes[i], NDAttrDataType_t::NDAttrUInt16); break;
            case pvxs::TypeCode::Int32:       toAttribute<int32_t>   (dest, attributes[i], NDAttrDataType_t::NDAttrInt32); break;
            case pvxs::TypeCode::UInt32:      toAttribute<uint32_t>  (dest, attributes[i], NDAttrDataType_t::NDAttrUInt32); break;
            case pvxs::TypeCode::Int64:       toAttribute<int64_t>   (dest, attributes[i], NDAttrDataType_t::NDAttrInt64); break;
            case pvxs::TypeCode::UInt64:      toAttribute<uint64_t>  (dest, attributes[i], NDAttrDataType_t::NDAttrUInt64); break;
            case pvxs::TypeCode::Float32:     toAttribute<float_t>   (dest, attributes[i], NDAttrDataType_t::NDAttrFloat32); break;
            case pvxs::TypeCode::Float64:     toAttribute<double_t>  (dest, attributes[i], NDAttrDataType_t::NDAttrFloat64); break;
            case pvxs::TypeCode::String:      toStringAttribute      (dest, attributes[i]); break;
            case pvxs::TypeCode::Null:        toUndefinedAttribute   (dest, attributes[i]); break;
            default: throw std::runtime_error("invalid value data type");
        }
    }
}

void NTNDArrayConverter::fromValue (NDArray *src) {
    NDArrayInfo_t arrayInfo;
    src->getInfo(&arrayInfo);

    m_value["compressedSize"] = src->compressedSize;
    m_value["uncompressedSize"] = arrayInfo.totalBytes;
    std::string field_name;
    pvxs::ArrayType arrayType;
    switch(src->dataType) {
        case NDDataType_t::NDInt8:      {arrayType = pvxs::ArrayType::Int8;     field_name = std::string("value->byteValue"); break;};
        case NDDataType_t::NDUInt8:     {arrayType = pvxs::ArrayType::UInt8;    field_name = std::string("value->ubyteValue"); break;};
        case NDDataType_t::NDInt16:     {arrayType = pvxs::ArrayType::Int16;    field_name = std::string("value->shortValue"); break;};
        case NDDataType_t::NDInt32:     {arrayType = pvxs::ArrayType::Int32;    field_name = std::string("value->intValue"); break;};
        case NDDataType_t::NDUInt32:    {arrayType = pvxs::ArrayType::UInt32;   field_name = std::string("value->uintValue"); break;};
        case NDDataType_t::NDInt64:     {arrayType = pvxs::ArrayType::Int64;    field_name = std::string("value->longValue"); break;};
        case NDDataType_t::NDUInt64:    {arrayType = pvxs::ArrayType::UInt64;   field_name = std::string("value->ulongValue"); break;};
        case NDDataType_t::NDFloat32:   {arrayType = pvxs::ArrayType::Float32;  field_name = std::string("value->floatValue"); break;};
        case NDDataType_t::NDFloat64:   {arrayType = pvxs::ArrayType::Float64;  field_name = std::string("value->doubleValue"); break;};
        default: {
            throw std::runtime_error("invalid value data type");
            break;
        }
    }
    const auto val = pvxs::detail::copyAs(
        arrayType, arrayType, (const void*) src->pData, arrayInfo.nElements).freeze();    
    m_value[field_name] = val;
    m_value["codec.name"] = src->codec.name; // compression codec
    // The uncompressed data type would be lost when converting to NTNDArray,
    // so we must store it somewhere. codec.parameters seems like a good place.
    m_value["codec.parameters"] = (int32_t) src->dataType;
}

void NTNDArrayConverter::fromDimensions (NDArray *src) {
    pvxs::shared_array<pvxs::Value> dims;
    dims.resize(src->ndims);

    for (int i = 0; i < src->ndims; i++) {
        dims[i] = m_value["dimension"].allocMember();
        dims[i].update("size", src->dims[i].size);
        dims[i].update("offset", src->dims[i].offset);
        dims[i].update("fullSize", src->dims[i].size);
        dims[i].update("binning", src->dims[i].binning);
        dims[i].update("reverse", src->dims[i].reverse);
    }
    m_value["dimension"] = dims.freeze();
}

void NTNDArrayConverter::fromDataTimeStamp (NDArray *src) {
    double seconds = floor(src->timeStamp);
    double nanoseconds = (src->timeStamp - seconds)*1e9;
    // pvAccess uses Posix time, NDArray uses EPICS time, need to convert
    seconds += POSIX_TIME_AT_EPICS_EPOCH;
    m_value["dataTimeStamp.secondsPastEpoch"] = seconds;
    m_value["dataTimeStamp.nanoseconds"] = nanoseconds;
}

void NTNDArrayConverter::fromTimeStamp (NDArray *src) {
    // pvAccess uses Posix time, NDArray uses EPICS time, need to convert
    m_value["timeStamp.secondsPastEpoch"] = src->epicsTS.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
    m_value["timeStamp.nanoseconds"] = src->epicsTS.nsec;
}

template <typename valueType>
void NTNDArrayConverter::fromAttribute (pvxs::Value dest_value, NDAttribute *src)
{
    valueType value;
    src->getValue(src->getDataType(), (void*)&value);
    dest_value["value"] = value;
}

void NTNDArrayConverter::fromStringAttribute (pvxs::Value dest_value, NDAttribute *src)
{
    const char *value;
    src->getValue(src->getDataType(), (void*)&value);
    dest_value["value"] = std::string(value);
}

void NTNDArrayConverter::fromAttributes (NDArray *src)
{
    NDAttributeList *srcList = src->pAttributeList;
    NDAttribute *attr = NULL;
    size_t i = 0;
    pvxs::shared_array<pvxs::Value> attrs;
    attrs.resize(src->pAttributeList->count());
    while((attr = srcList->next(attr)))
    {
        NDAttrSource_t sourceType;
        attr->getSourceInfo(&sourceType);
        attrs[i] = m_value["attribute"].allocMember();
        attrs[i].update("name", attr->getName());
        attrs[i].update("descriptor", attr->getDescription());
        attrs[i].update("source", attr->getSource());
        attrs[i].update("sourceType", sourceType);

        switch(attr->getDataType())
        {
        case NDAttrInt8:      fromAttribute<int8_t>(attrs[i], attr);       break;
        case NDAttrUInt8:     fromAttribute<uint8_t>(attrs[i], attr);      break;
        case NDAttrInt16:     fromAttribute<int16_t>(attrs[i], attr);      break;
        case NDAttrUInt16:    fromAttribute<uint16_t>(attrs[i], attr);     break;
        case NDAttrInt32:     fromAttribute<int32_t>(attrs[i], attr);      break;
        case NDAttrUInt32:    fromAttribute<uint32_t>(attrs[i], attr);     break;
        case NDAttrInt64:     fromAttribute<int64_t>(attrs[i], attr);      break;
        case NDAttrUInt64:    fromAttribute<uint64_t>(attrs[i], attr);     break;
        case NDAttrFloat32:   fromAttribute<float>(attrs[i], attr);        break;
        case NDAttrFloat64:   fromAttribute<double>(attrs[i], attr);       break;
        case NDAttrString:    fromStringAttribute(attrs[i], attr);         break;
        case NDAttrUndefined: break;  // No need to assign value, leave as undefined
        default:              throw std::runtime_error("invalid attribute data type");
        }
        ++i;
    }
    m_value["attribute"] = attrs.freeze();
}




