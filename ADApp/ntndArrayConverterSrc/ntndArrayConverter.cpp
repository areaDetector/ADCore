#include <math.h>
#include <epicsTime.h>

#include <epicsExport.h>
#include "ntndArrayConverter.h"

using namespace std;
using namespace epics::nt;
using namespace epics::pvData;
using tr1::static_pointer_cast;

// Maps the selected index of the value field to its type.
static const NDDataType_t scalarToNDDataType[pvString+1] = {
        NDInt8,     // 0:  pvBoolean (not supported)
        NDInt8,     // 1:  pvByte
        NDInt16,    // 2:  pvShort
        NDInt32,    // 3:  pvInt
        NDInt64,    // 4:  pvLong
        NDUInt8,    // 5:  pvUByte
        NDUInt16,   // 6:  pvUShort
        NDUInt32,   // 7:  pvUInt
        NDUInt64,   // 8:  pvULong
        NDFloat32,  // 9:  pvFloat
        NDFloat64,  // 10: pvDouble
        NDInt8,     // 11: pvString (notSupported)
};

// Maps ScalarType to NDAttrDataType_t
static const NDAttrDataType_t scalarToNDAttrDataType[pvString+1] = {
        NDAttrInt8,     // 0:  pvBoolean (not supported)
        NDAttrInt8,     // 1:  pvByte
        NDAttrInt16,    // 2:  pvShort
        NDAttrInt32,    // 3:  pvInt
        NDAttrInt64,    // 4:  pvLong
        NDAttrUInt8,    // 5:  pvUByte
        NDAttrUInt16,   // 6:  pvUShort
        NDAttrUInt32,   // 7:  pvUInt
        NDAttrUInt64,   // 8:  pvULong
        NDAttrFloat32,  // 9:  pvFloat
        NDAttrFloat64,  // 10: pvDouble
        NDAttrString,   // 11: pvString
};

// Maps NDDataType to ScalarType
static const ScalarType NDDataTypeToScalar[NDFloat64 + 1] = {
        pvByte,     // 0:  NDInt8
        pvUByte,    // 1:  NDUInt8
        pvShort,    // 2:  NDInt16
        pvUShort,   // 3:  NDUInt16
        pvInt,      // 4:  NDInt32
        pvUInt,     // 5:  NDUInt32
        pvLong,     // 6:  NDInt32
        pvULong,    // 7:  NDUInt32
        pvFloat,    // 8:  NDFloat32
        pvDouble,   // 9:  NDFloat64
};

static const PVDataCreatePtr PVDC = getPVDataCreate();

template <typename dataType>
struct freeNDArray {
    NDArray *array;
    freeNDArray(NDArray *array) : array(array) {};
    void operator()(dataType *data) { array->release(); }
};

NTNDArrayConverter::NTNDArrayConverter (NTNDArrayPtr array) : m_array(array) {}

ScalarType NTNDArrayConverter::getValueType (void)
{
    string fieldName(m_array->getValue()->getSelectedFieldName());

    /*
     * Check if union field selected. It happens when the driver is run before
     * the producer. There is a monitor update that is sent on the
     * initialization of a PVRecord with no real data.
     */
    if(fieldName.empty())
        throw std::runtime_error("no union field selected");

    string typeName(fieldName.substr(0,fieldName.find("Value")));
    return ScalarTypeFunc::getScalarType(typeName);
}

NDColorMode_t NTNDArrayConverter::getColorMode (void)
{
    NDColorMode_t colorMode = NDColorModeMono;
    PVStructureArray::const_svector attrs(m_array->getAttribute()->view());

    for(PVStructureArray::const_svector::iterator it(attrs.cbegin());
            it != attrs.cend(); ++it)
    {
        PVStringPtr nameFld((*it)->getSubFieldT<PVString>("name"));
        if(nameFld->get() == "ColorMode")
        {
            PVUnionPtr valueUnion((*it)->getSubFieldT<PVUnion>("value"));
            PVScalar::shared_pointer valueFld(valueUnion->get<PVScalar>());
            if(valueFld) {
                int cm = valueFld->getAs<int32>();
                colorMode = (NDColorMode_t) cm;
            } else
                throw std::runtime_error("Error accessing attribute ColorMode");
        }
    }

    return colorMode;
}

NTNDArrayInfo_t NTNDArrayConverter::getInfo (void)
{
    NTNDArrayInfo_t info = {0};

    PVStructureArray::const_svector dims(m_array->getDimension()->view());

    info.ndims     = (int) dims.size();
    info.nElements = 1;

    for(int i = 0; i < info.ndims; ++i)
    {
        info.dims[i]    = (size_t) dims[i]->getSubField<PVInt>("size")->get();
        info.nElements *= info.dims[i];
    }

    PVStructurePtr codec(m_array->getCodec());

    info.codec = codec->getSubField<PVString>("name")->get();

    ScalarType dataType;

    if (info.codec.empty())
        dataType = getValueType();
    else {
        // Read uncompressed data type
        PVIntPtr udt(codec->getSubField<PVUnion>("parameters")->get<PVInt>());
        dataType = static_cast<ScalarType>(udt->get());
    }

    NDDataType_t dt;
    int bpe;
    switch(dataType)
    {
    case pvByte:    dt = NDInt8;     bpe = sizeof(epicsInt8);    break;
    case pvUByte:   dt = NDUInt8;    bpe = sizeof(epicsUInt8);   break;
    case pvShort:   dt = NDInt16;    bpe = sizeof(epicsInt16);   break;
    case pvUShort:  dt = NDUInt16;   bpe = sizeof(epicsUInt16);  break;
    case pvInt:     dt = NDInt32;    bpe = sizeof(epicsInt32);   break;
    case pvUInt:    dt = NDUInt32;   bpe = sizeof(epicsUInt32);  break;
    case pvLong:    dt = NDInt64;    bpe = sizeof(epicsInt64);   break;
    case pvULong:   dt = NDUInt64;   bpe = sizeof(epicsUInt64);  break;
    case pvFloat:   dt = NDFloat32;  bpe = sizeof(epicsFloat32); break;
    case pvDouble:  dt = NDFloat64;  bpe = sizeof(epicsFloat64); break;
    case pvBoolean:
    case pvString:
    default:
        throw std::runtime_error("invalid value data type");
        break;
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

    // getUniqueId not implemented yet
    // dest->uniqueId = m_array->getUniqueId()->get();
    PVIntPtr uniqueId(m_array->getPVStructure()->getSubField<PVInt>("uniqueId"));
    dest->uniqueId = uniqueId->get();
}

void NTNDArrayConverter::fromArray (NDArray *src)
{
    fromValue(src);
    fromDimensions(src);
    fromTimeStamp(src);
    fromDataTimeStamp(src);
    fromAttributes(src);

    // getUniqueId not implemented yet
    // m_array->getUniqueId()->put(src->uniqueId);
    PVIntPtr uniqueId(m_array->getPVStructure()->getSubField<PVInt>("uniqueId"));
    uniqueId->put(src->uniqueId);
}

template <typename arrayType>
void NTNDArrayConverter::toValue (NDArray *dest)
{
    typedef typename arrayType::value_type arrayValType;
    typedef typename arrayType::const_svector arrayVecType;

    PVUnionPtr src(m_array->getValue());
    arrayVecType srcVec(src->get<arrayType>()->view());
    memcpy(dest->pData, srcVec.data(), srcVec.size()*sizeof(arrayValType));

    NTNDArrayInfo_t info = getInfo();
    dest->codec.name = info.codec;
    dest->dataType = info.dataType;

    if (!info.codec.empty())
        dest->compressedSize = srcVec.size()*sizeof(arrayValType);
}

void NTNDArrayConverter::toValue (NDArray *dest)
{
    switch(getValueType())
    {
    case pvByte:    toValue<PVByteArray>  (dest); break;
    case pvUByte:   toValue<PVUByteArray> (dest); break;
    case pvShort:   toValue<PVShortArray> (dest); break;
    case pvUShort:  toValue<PVUShortArray>(dest); break;
    case pvInt:     toValue<PVIntArray>   (dest); break;
    case pvUInt:    toValue<PVUIntArray>  (dest); break;
    case pvFloat:   toValue<PVFloatArray> (dest); break;
    case pvDouble:  toValue<PVDoubleArray>(dest); break;
    case pvBoolean:
    case pvLong:
    case pvULong:
    case pvString:
    default:
        throw std::runtime_error("invalid value data type");
        break;
    }

}

void NTNDArrayConverter::toDimensions (NDArray *dest)
{
    PVStructureArrayPtr src(m_array->getDimension());
    PVStructureArray::const_svector srcVec(src->view());

    dest->ndims = (int)srcVec.size();

    for(size_t i = 0; i < srcVec.size(); ++i)
    {
        NDDimension_t *d = &dest->dims[i];
        d->size    = srcVec[i]->getSubField<PVInt>("size")->get();
        d->offset  = srcVec[i]->getSubField<PVInt>("offset")->get();
        d->binning = srcVec[i]->getSubField<PVInt>("binning")->get();
        d->reverse = srcVec[i]->getSubField<PVBoolean>("reverse")->get();
    }
}

void NTNDArrayConverter::toTimeStamp (NDArray *dest)
{
    PVStructurePtr src(m_array->getTimeStamp());

    if(!src.get())
        return;

    PVTimeStamp pvSrc;
    pvSrc.attach(src);

    TimeStamp ts;
    pvSrc.get(ts);

    dest->epicsTS.secPastEpoch = (epicsUInt32)ts.getSecondsPastEpoch();
    dest->epicsTS.nsec = ts.getNanoseconds();
}

void NTNDArrayConverter::toDataTimeStamp (NDArray *dest)
{
    PVStructurePtr src(m_array->getDataTimeStamp());
    PVTimeStamp pvSrc;
    pvSrc.attach(src);

    TimeStamp ts;
    pvSrc.get(ts);

    dest->timeStamp = ts.toSeconds();
}

template <typename pvAttrType, typename valueType>
void NTNDArrayConverter::toAttribute (NDArray *dest, PVStructurePtr src)
{
    const char *name          = src->getSubField<PVString>("name")->get().c_str();
    const char *desc          = src->getSubField<PVString>("descriptor")->get().c_str();
    NDAttrSource_t sourceType = (NDAttrSource_t)src->getSubField<PVInt>("sourceType")->get();
    const char *source        = src->getSubField<PVString>("source")->get().c_str();
    NDAttrDataType_t dataType = scalarToNDAttrDataType[pvAttrType::typeCode];
    valueType value           = src->getSubField<PVUnion>("value")->get<pvAttrType>()->get();

    NDAttribute *attr = new NDAttribute(name, desc, sourceType, source, dataType, (void*)&value);
    dest->pAttributeList->add(attr);
}

void NTNDArrayConverter::toStringAttribute (NDArray *dest, PVStructurePtr src)
{
    const char *name          = src->getSubField<PVString>("name")->get().c_str();
    const char *desc          = src->getSubField<PVString>("descriptor")->get().c_str();
    NDAttrSource_t sourceType = (NDAttrSource_t)src->getSubField<PVInt>("sourceType")->get();
    const char *source        = src->getSubField<PVString>("source")->get().c_str();
    const char *value         = src->getSubField<PVUnion>("value")->get<PVString>()->get().c_str();

    NDAttribute *attr = new NDAttribute(name, desc, sourceType, source, NDAttrString, (void*)value);
    dest->pAttributeList->add(attr);
}

void NTNDArrayConverter::toUndefinedAttribute (NDArray *dest, PVStructurePtr src)
{
    const char *name          = src->getSubField<PVString>("name")->get().c_str();
    const char *desc          = src->getSubField<PVString>("descriptor")->get().c_str();
    NDAttrSource_t sourceType = (NDAttrSource_t)src->getSubField<PVInt>("sourceType")->get();
    const char *source        = src->getSubField<PVString>("source")->get().c_str();

    NDAttribute *attr = new NDAttribute(name, desc, sourceType, source, NDAttrUndefined, NULL);
    dest->pAttributeList->add(attr);
}

void NTNDArrayConverter::toAttributes (NDArray *dest)
{
    typedef PVStructureArray::const_svector::const_iterator VecIt;

    PVStructureArray::const_svector srcVec(m_array->getAttribute()->view());

    for(VecIt it = srcVec.cbegin(); it != srcVec.cend(); ++it)
    {
        PVScalarPtr srcScalar((*it)->getSubField<PVUnion>("value")->get<PVScalar>());

        if(!srcScalar)
            toUndefinedAttribute(dest, *it);
        else
        {
            switch(srcScalar->getScalar()->getScalarType())
            {
            case pvByte:   toAttribute<PVByte,   int8_t>  (dest, *it); break;
            case pvUByte:  toAttribute<PVUByte,  uint8_t> (dest, *it); break;
            case pvShort:  toAttribute<PVShort,  int16_t> (dest, *it); break;
            case pvUShort: toAttribute<PVUShort, uint16_t>(dest, *it); break;
            case pvInt:    toAttribute<PVInt,    int32_t> (dest, *it); break;
            case pvUInt:   toAttribute<PVUInt,   uint32_t>(dest, *it); break;
            case pvLong:   toAttribute<PVLong,   int64_t> (dest, *it); break;
            case pvULong:  toAttribute<PVULong,  uint64_t>(dest, *it); break;
            case pvFloat:  toAttribute<PVFloat,  float>   (dest, *it); break;
            case pvDouble: toAttribute<PVDouble, double>  (dest, *it); break;
            case pvString: toStringAttribute (dest, *it); break;
            case pvBoolean:
            default:
                break;   // ignore invalid types
            }
        }
    }
}

template <typename arrayType, typename srcDataType>
void NTNDArrayConverter::fromValue (NDArray *src)
{
    typedef typename arrayType::value_type arrayValType;

    string unionField(string(ScalarTypeFunc::name(arrayType::typeCode)) +
            string("Value"));

    NDArrayInfo_t arrayInfo;
    src->getInfo(&arrayInfo);

    int64 compressedSize = src->compressedSize;
    int64 uncompressedSize = arrayInfo.totalBytes;

    m_array->getCompressedDataSize()->put(compressedSize);
    m_array->getUncompressedDataSize()->put(uncompressedSize);

    // The uncompressed data type would be lost when converting to NTNDArray,
    // so we must store it somewhere. codec.parameters seems like a good place.
    PVScalarPtr uncompressedType(PVDC->createPVScalar(pvInt));
    uncompressedType->putFrom<int32>(NDDataTypeToScalar[src->dataType]);

    PVStructurePtr codec(m_array->getCodec());
    codec->getSubField<PVUnion>("parameters")->set(uncompressedType);
    codec->getSubField<PVString>("name")->put(src->codec.name);

    size_t count = src->codec.empty() ? arrayInfo.nElements : compressedSize;

    src->reserve();
    shared_vector<arrayValType> temp((srcDataType*)src->pData,
            freeNDArray<srcDataType>(src), 0, count);

    PVUnionPtr dest = m_array->getValue();
    dest->select<arrayType>(unionField)->replace(freeze(temp));
    dest->postPut();
}

void NTNDArrayConverter::fromValue (NDArray *src)
{
    // Uncompressed
    if (src->codec.empty()) {
        switch(src->dataType)
        {
        case NDInt8:    fromValue<PVByteArray,   int8_t>   (src); break;
        case NDUInt8:   fromValue<PVUByteArray,  uint8_t>  (src); break;
        case NDInt16:   fromValue<PVShortArray,  int16_t>  (src); break;
        case NDUInt16:  fromValue<PVUShortArray, uint16_t> (src); break;
        case NDInt32:   fromValue<PVIntArray,    int32_t>  (src); break;
        case NDUInt32:  fromValue<PVUIntArray,   uint32_t> (src); break;
        case NDInt64:   fromValue<PVLongArray,   int64_t>  (src); break;
        case NDUInt64:  fromValue<PVULongArray,  uint64_t> (src); break;
        case NDFloat32: fromValue<PVFloatArray,  float>    (src); break;
        case NDFloat64: fromValue<PVDoubleArray, double>   (src); break;
        }
    // Compressed
    } else {
        fromValue<PVUByteArray, uint8_t>(src);
    }
}

void NTNDArrayConverter::fromDimensions (NDArray *src)
{
    PVStructureArrayPtr dest(m_array->getDimension());
    PVStructureArray::svector destVec(dest->reuse());
    StructureConstPtr dimStructure(dest->getStructureArray()->getStructure());

    destVec.resize(src->ndims);
    for (int i = 0; i < src->ndims; i++)
    {
        if (!destVec[i] || !destVec[i].unique())
            destVec[i] = PVDC->createPVStructure(dimStructure);

        destVec[i]->getSubField<PVInt>("size")->put((int)src->dims[i].size);
        destVec[i]->getSubField<PVInt>("offset")->put((int)src->dims[i].offset);
        destVec[i]->getSubField<PVInt>("fullSize")->put((int)src->dims[i].size);
        destVec[i]->getSubField<PVInt>("binning")->put(src->dims[i].binning);
        destVec[i]->getSubField<PVBoolean>("reverse")->put(src->dims[i].reverse);
    }
    dest->replace(freeze(destVec));
}

void NTNDArrayConverter::fromDataTimeStamp (NDArray *src)
{
    PVStructurePtr dest(m_array->getDataTimeStamp());

    double seconds = floor(src->timeStamp);
    double nanoseconds = (src->timeStamp - seconds)*1e9;
    // pvAccess uses Posix time, NDArray uses EPICS time, need to convert
    seconds += POSIX_TIME_AT_EPICS_EPOCH;

    PVTimeStamp pvDest;
    pvDest.attach(dest);

    TimeStamp ts((int64_t)seconds, (int32_t)nanoseconds);
    pvDest.set(ts);
}

void NTNDArrayConverter::fromTimeStamp (NDArray *src)
{
    PVStructurePtr dest(m_array->getTimeStamp());

    PVTimeStamp pvDest;
    pvDest.attach(dest);

    // pvAccess uses Posix time, NDArray uses EPICS time, need to convert
    TimeStamp ts(src->epicsTS.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, src->epicsTS.nsec);
    pvDest.set(ts);
}

template <typename pvAttrType, typename valueType>
void NTNDArrayConverter::fromAttribute (PVStructurePtr dest, NDAttribute *src)
{
    valueType value;
    src->getValue(src->getDataType(), (void*)&value);

    PVUnionPtr destUnion(dest->getSubFieldT<PVUnion>("value"));
    typename pvAttrType::shared_pointer valueFld(destUnion->get<pvAttrType>());
    if(!valueFld) {
        valueFld = PVDC->createPVScalar<pvAttrType>();
        destUnion->set(valueFld);
    }
    valueFld->put(value);
}

void NTNDArrayConverter::fromStringAttribute (PVStructurePtr dest, NDAttribute *src)
{
    NDAttrDataType_t attrDataType;
    size_t attrDataSize;

    src->getValueInfo(&attrDataType, &attrDataSize);
    std::vector<char> value(attrDataSize);
    src->getValue(attrDataType, &value[0], attrDataSize);

    PVUnionPtr destUnion(dest->getSubFieldT<PVUnion>("value"));
    PVStringPtr valueFld(destUnion->get<PVString>());
    if(!valueFld) {
        valueFld = PVDC->createPVScalar<PVString>();
        destUnion->set(valueFld);
    }
    valueFld->put(&value[0]);
}

void NTNDArrayConverter::fromUndefinedAttribute (PVStructurePtr dest)
{
    PVFieldPtr nullPtr;
    dest->getSubField<PVUnion>("value")->set(nullPtr);
}

void NTNDArrayConverter::fromAttributes (NDArray *src)
{
    PVStructureArrayPtr dest(m_array->getAttribute());
    NDAttributeList *srcList = src->pAttributeList;
    NDAttribute *attr = NULL;
    StructureConstPtr structure(dest->getStructureArray()->getStructure());
    PVStructureArray::svector destVec(dest->reuse());

    destVec.resize(srcList->count());

    size_t i = 0;
    while((attr = srcList->next(attr)))
    {
        if(!destVec[i].get() || !destVec[i].unique())
            destVec[i] = PVDC->createPVStructure(structure);

        PVStructurePtr pvAttr(destVec[i]);

        pvAttr->getSubField<PVString>("name")->put(attr->getName());
        pvAttr->getSubField<PVString>("descriptor")->put(attr->getDescription());
        pvAttr->getSubField<PVString>("source")->put(attr->getSource());

        NDAttrSource_t sourceType;
        attr->getSourceInfo(&sourceType);
        pvAttr->getSubField<PVInt>("sourceType")->put(sourceType);

        switch(attr->getDataType())
        {
        case NDAttrInt8:      fromAttribute <PVByte,   int8_t>  (pvAttr, attr); break;
        case NDAttrUInt8:     fromAttribute <PVUByte,  uint8_t> (pvAttr, attr); break;
        case NDAttrInt16:     fromAttribute <PVShort,  int16_t> (pvAttr, attr); break;
        case NDAttrUInt16:    fromAttribute <PVUShort, uint16_t>(pvAttr, attr); break;
        case NDAttrInt32:     fromAttribute <PVInt,    int32_t> (pvAttr, attr); break;
        case NDAttrUInt32:    fromAttribute <PVUInt,   uint32_t>(pvAttr, attr); break;
        case NDAttrInt64:     fromAttribute <PVLong,   int64_t> (pvAttr, attr); break;
        case NDAttrUInt64:    fromAttribute <PVULong,  uint64_t>(pvAttr, attr); break;
        case NDAttrFloat32:   fromAttribute <PVFloat,  float>   (pvAttr, attr); break;
        case NDAttrFloat64:   fromAttribute <PVDouble, double>  (pvAttr, attr); break;
        case NDAttrString:    fromStringAttribute(pvAttr, attr); break;
        case NDAttrUndefined: fromUndefinedAttribute(pvAttr); break;
        default:              throw std::runtime_error("invalid attribute data type");
        }

        ++i;
    }

    dest->replace(freeze(destVec));
}




