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

NTNDArrayConverter::NTNDArrayConverter (pvxs::Value value) : m_value(value) {
    m_typeMap = {
        {typeid(int8_t), NDAttrDataType_t::NDAttrInt8},
        {typeid(uint8_t), NDAttrDataType_t::NDAttrUInt8},
        {typeid(int16_t), NDAttrDataType_t::NDAttrInt16},
        {typeid(uint16_t), NDAttrDataType_t::NDAttrUInt16},
        {typeid(int32_t), NDAttrDataType_t::NDAttrInt32},
        {typeid(uint32_t), NDAttrDataType_t::NDAttrUInt32},
        {typeid(int64_t), NDAttrDataType_t::NDAttrInt64},
        {typeid(uint64_t), NDAttrDataType_t::NDAttrUInt64},
        {typeid(float_t), NDAttrDataType_t::NDAttrFloat32},
        {typeid(double_t), NDAttrDataType_t::NDAttrFloat64}
    };

    m_fieldNameMap = {
        {typeid(int8_t), "value->byteValue"},
        {typeid(uint8_t), "value->ubyteValue"},
        {typeid(int16_t), "value->shortValue"},
        {typeid(uint16_t), "value->ushortValue"},
        {typeid(int32_t), "value->intValue"},
        {typeid(uint32_t), "value->uintValue"},
        {typeid(int64_t), "value->longValue"},
        {typeid(uint64_t), "value->ulongValue"},
        {typeid(float_t), "value->floatValue"},
        {typeid(double_t), "value->doubleValue"}
    };

    m_arrayTypeMap = {
        {typeid(int8_t), pvxs::ArrayType::Int8},
        {typeid(uint8_t), pvxs::ArrayType::UInt8},
        {typeid(int16_t), pvxs::ArrayType::Int16},
        {typeid(int32_t), pvxs::ArrayType::Int32},
        {typeid(uint32_t), pvxs::ArrayType::UInt32},
        {typeid(int64_t), pvxs::ArrayType::Int64},
        {typeid(uint64_t), pvxs::ArrayType::UInt64},
        {typeid(float_t), pvxs::ArrayType::Float32},
        {typeid(double_t), pvxs::ArrayType::Float64}
    };

}

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
        switch (m_value["value->"].type().code) {
        case pvxs::TypeCode::Int8A:      {dt = NDInt8;      bpe = sizeof(int8_t);   break;}
        case pvxs::TypeCode::UInt8A:     {dt = NDUInt8;     bpe = sizeof(uint8_t);  break;}
        case pvxs::TypeCode::Int16A:     {dt = NDInt16;     bpe = sizeof(int16_t);  break;}
        case pvxs::TypeCode::UInt16A:    {dt = NDUInt16;    bpe = sizeof(uint16_t); break;}
        case pvxs::TypeCode::Int32A:     {dt = NDInt32;     bpe = sizeof(int32_t);  break;}
        case pvxs::TypeCode::UInt32A:    {dt = NDUInt32;    bpe = sizeof(uint32_t); break;}
        case pvxs::TypeCode::Int64A:     {dt = NDInt64;     bpe = sizeof(int64_t);  break;}
        case pvxs::TypeCode::UInt64A:    {dt = NDUInt64;    bpe = sizeof(uint64_t); break;}
        case pvxs::TypeCode::Float32A:   {dt = NDFloat32;   bpe = sizeof(float_t);  break;}
        case pvxs::TypeCode::Float64A:   {dt = NDFloat64;   bpe = sizeof(double_t); break;}
        default: throw std::runtime_error("invalid value data type");
        }
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
void NTNDArrayConverter::toValue (NDArray *dest)
{
    NTNDArrayInfo_t info = getInfo();
    dest->codec.name = info.codec;
    dest->dataType = info.dataType;


    std::string fieldName = m_fieldNameMap[typeid(arrayType)];

    auto value = m_value[fieldName].as<pvxs::shared_array<const arrayType>>();
    memcpy(dest->pData, value.data(), info.totalBytes);

    if (!info.codec.empty())
        dest->compressedSize = info.totalBytes;
}

void NTNDArrayConverter::toValue (NDArray *dest)
{
    switch (m_value["value->"].type().code) {
    case pvxs::TypeCode::Int8A:     {toValue<int8_t>(dest); break;}
    case pvxs::TypeCode::UInt8A:    {toValue<uint8_t>(dest); break;}
    case pvxs::TypeCode::Int16A:    {toValue<int16_t>(dest); break;}
    case pvxs::TypeCode::UInt16A:   {toValue<uint16_t>(dest); break;}
    case pvxs::TypeCode::Int32A:    {toValue<int32_t>(dest); break;}
    case pvxs::TypeCode::UInt32A:   {toValue<uint32_t>(dest); break;}
    case pvxs::TypeCode::Int64A:    {toValue<int64_t>(dest); break;}
    case pvxs::TypeCode::UInt64A:   {toValue<uint64_t>(dest); break;}
    case pvxs::TypeCode::Float32A:  {toValue<float_t>(dest); break;}
    case pvxs::TypeCode::Float64A:  {toValue<double_t>(dest); break;}
    default: throw std::runtime_error("invalid value data type");
    }
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
void NTNDArrayConverter::toAttribute (NDArray *dest, pvxs::Value attribute)
{
    auto name = attribute["name"].as<std::string>();
    auto desc = attribute["descriptor"].as<std::string>();
    auto source = attribute["source"].as<std::string>();
    NDAttrSource_t sourceType = (NDAttrSource_t) attribute["sourceType"].as<int32_t>();
    valueType value = attribute["value"].as<valueType>();
    NDAttrDataType_t dataType = m_typeMap[typeid(valueType)];

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
            case pvxs::TypeCode::Int8:        toAttribute<int8_t>    (dest, attributes[i]); break;
            case pvxs::TypeCode::UInt8:       toAttribute<uint8_t>   (dest, attributes[i]); break;
            case pvxs::TypeCode::Int16:       toAttribute<int16_t>   (dest, attributes[i]); break;
            case pvxs::TypeCode::UInt16:      toAttribute<uint16_t>  (dest, attributes[i]); break;
            case pvxs::TypeCode::Int32:       toAttribute<int32_t>   (dest, attributes[i]); break;
            case pvxs::TypeCode::UInt32:      toAttribute<uint32_t>  (dest, attributes[i]); break;
            case pvxs::TypeCode::Int64:       toAttribute<int64_t>   (dest, attributes[i]); break;
            case pvxs::TypeCode::UInt64:      toAttribute<uint64_t>  (dest, attributes[i]); break;
            case pvxs::TypeCode::Float32:     toAttribute<float_t>   (dest, attributes[i]); break;
            case pvxs::TypeCode::Float64:     toAttribute<double_t>  (dest, attributes[i]); break;
            case pvxs::TypeCode::String:      toStringAttribute      (dest, attributes[i]); break;
            case pvxs::TypeCode::Null:        toUndefinedAttribute   (dest, attributes[i]); break;
            default: throw std::runtime_error("invalid value data type");
        }
    }
}

template <typename dataType>
void NTNDArrayConverter::fromValue(NDArray *src) {
    NDArrayInfo_t arrayInfo;
    src->getInfo(&arrayInfo);

    m_value["compressedSize"] = src->compressedSize;
    m_value["uncompressedSize"] = arrayInfo.totalBytes;
    std::string fieldName = m_fieldNameMap[typeid(dataType)];
    auto arrayType = m_arrayTypeMap[typeid(dataType)];
    const auto val = pvxs::detail::copyAs(
        arrayType, arrayType, (const void*) src->pData, arrayInfo.nElements).freeze();
    m_value[fieldName] = val;
    m_value["codec.name"] = src->codec.name; // compression codec
    // The uncompressed data type would be lost when converting to NTNDArray,
    // so we must store it somewhere. codec.parameters seems like a good place.
    m_value["codec.parameters"] = (int32_t) src->dataType;
}

void NTNDArrayConverter::fromValue (NDArray *src) {
    switch(src->dataType) {
        case NDDataType_t::NDInt8:      {fromValue<int8_t>(src); break;};
        case NDDataType_t::NDUInt8:     {fromValue<uint8_t>(src); break;};
        case NDDataType_t::NDInt16:     {fromValue<int16_t>(src); break;};
        case NDDataType_t::NDInt32:     {fromValue<int32_t>(src); break;};
        case NDDataType_t::NDUInt32:    {fromValue<uint32_t>(src); break;};
        case NDDataType_t::NDInt64:     {fromValue<int64_t>(src); break;};
        case NDDataType_t::NDUInt64:    {fromValue<uint64_t>(src); break;};
        case NDDataType_t::NDFloat32:   {fromValue<float_t>(src); break;};
        case NDDataType_t::NDFloat64:   {fromValue<double_t>(src); break;};
        default: {
            throw std::runtime_error("invalid value data type");
            break;
        }
    }
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




