#include "ntndArrayConverter.h"
#include <math.h>

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
        NDInt8,     // 4:  pvLong (not supported)
        NDUInt8,    // 5:  pvUByte
        NDUInt16,   // 6:  pvUShort
        NDUInt32,   // 7:  pvUInt
        NDInt8,     // 8:  pvULong (not supported)
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
        NDAttrInt8,     // 4:  pvLong (not supported)
        NDAttrUInt8,    // 5:  pvUByte
        NDAttrUInt16,   // 6:  pvUShort
        NDAttrUInt32,   // 7:  pvUInt
        NDAttrInt8,     // 8:  pvULong (not supported)
        NDAttrFloat32,  // 9:  pvFloat
        NDAttrFloat64,  // 10: pvDouble
        NDAttrString,   // 11: pvString
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
        if((*it)->getSubField<PVString>("name")->get() == "ColorMode")
        {
            PVUnionPtr field((*it)->getSubField<PVUnion>("value"));
            int cm = static_pointer_cast<PVInt>(field->get())->get();
            colorMode = (NDColorMode_t) cm;
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

    NDDataType_t dt;
    int bpe;
    switch(getValueType())
    {
    case pvByte:    dt = NDInt8;     bpe = sizeof(epicsInt8);    break;
    case pvUByte:   dt = NDUInt8;    bpe = sizeof(epicsUInt8);   break;
    case pvShort:   dt = NDInt16;    bpe = sizeof(epicsInt16);   break;
    case pvUShort:  dt = NDUInt16;   bpe = sizeof(epicsUInt16);  break;
    case pvInt:     dt = NDInt32;    bpe = sizeof(epicsInt32);   break;
    case pvUInt:    dt = NDUInt32;   bpe = sizeof(epicsUInt32);  break;
    case pvFloat:   dt = NDFloat32;  bpe = sizeof(epicsFloat32); break;
    case pvDouble:  dt = NDFloat64;  bpe = sizeof(epicsFloat64); break;
    case pvBoolean:
    case pvLong:
    case pvULong:
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

    m_array->getCodec()->getSubField<PVString>("name")->put("");
    m_array->getCompressedDataSize()->put(static_cast<int64>(src->dataSize));
    m_array->getUncompressedDataSize()->put(static_cast<int64>(src->dataSize));

    // getUniqueId not implemented yet
    // m_array->getUniqueId()->put(src->uniqueId);
    PVIntPtr uniqueId(m_array->getPVStructure()->getSubField<PVInt>("uniqueId"));
    uniqueId->put(src->uniqueId);
}

template <typename arrayType>
void NTNDArrayConverter::toValue (NDArray *dest)
{
    PVUnionPtr src(m_array->getValue());
    memcpy(dest->pData, src->get<arrayType>()->view().data(), dest->dataSize);
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

    dest->ndims = srcVec.size();

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

    dest->epicsTS.secPastEpoch = ts.getSecondsPastEpoch();
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
    NDAttributeList *destList = dest->pAttributeList;
    NDAttrDataType_t destType = scalarToNDAttrDataType[pvAttrType::typeCode];
    PVUnionPtr valueUnion(src->getSubField<PVUnion>("value"));
    valueType value = valueUnion->get<pvAttrType>()->get();
    const char *name = src->getSubField<PVString>("name")->get().c_str();
    const char *desc = src->getSubField<PVString>("descriptor")->get().c_str();
    // sourceType and source are lost

    destList->add(name, desc, destType, (void*)&value);
}

void NTNDArrayConverter::toStringAttribute (NDArray *dest, PVStructurePtr src)
{
    NDAttributeList *destList = dest->pAttributeList;
    PVUnionPtr valueUnion(src->getSubField<PVUnion>("value"));
    string value(valueUnion->get<PVString>()->get());
    const char *name = src->getSubField<PVString>("name")->get().c_str();
    const char *desc = src->getSubField<PVString>("descriptor")->get().c_str();
    // sourceType and source are lost

    destList->add(name, desc, NDAttrString, (void*)value.c_str());
}

void NTNDArrayConverter::toAttributes (NDArray *dest)
{
    typedef PVStructureArray::const_svector::const_iterator VecIt;

    PVStructureArray::const_svector srcVec(m_array->getAttribute()->view());

    for(VecIt it = srcVec.cbegin(); it != srcVec.cend(); ++it)
    {
        PVUnionPtr srcUnion((*it)->getSubField<PVUnion>("value"));
        ScalarConstPtr srcScalar(srcUnion->get<PVScalar>()->getScalar());

        switch(srcScalar->getScalarType())
        {
        case pvByte:   toAttribute<PVByte,   int8_t>  (dest, *it); break;
        case pvUByte:  toAttribute<PVUByte,  uint8_t> (dest, *it); break;
        case pvShort:  toAttribute<PVShort,  int16_t> (dest, *it); break;
        case pvUShort: toAttribute<PVUShort, uint16_t>(dest, *it); break;
        case pvInt:    toAttribute<PVInt,    int32_t> (dest, *it); break;
        case pvUInt:   toAttribute<PVUInt,   uint32_t>(dest, *it); break;
        case pvFloat:  toAttribute<PVFloat,  float>   (dest, *it); break;
        case pvDouble: toAttribute<PVDouble, double>  (dest, *it); break;
        case pvString: toStringAttribute (dest, *it); break;
        case pvBoolean:
        case pvLong:
        case pvULong:
        default:
            break;   // ignore invalid types
        }
    }
}

template <typename arrayType, typename srcDataType>
void NTNDArrayConverter::fromValue (NDArray *src)
{
    typedef typename arrayType::value_type arrayValType;

    NDArrayInfo_t arrayInfo;
    size_t count;

    string unionField(string(ScalarTypeFunc::name(arrayType::typeCode)) +
            string("Value"));

    src->getInfo(&arrayInfo);
    count = arrayInfo.nElements;

    src->reserve();
    shared_vector<arrayValType> temp((srcDataType*)src->pData,
            freeNDArray<srcDataType>(src), 0, count);

    PVUnionPtr dest = m_array->getValue();
    dest->select<arrayType>(unionField)->replace(freeze(temp));
    dest->postPut();
}

void NTNDArrayConverter::fromValue (NDArray *src)
{
    switch(src->dataType)
    {
    case NDInt8:    fromValue<PVByteArray,   int8_t>   (src); break;
    case NDUInt8:   fromValue<PVUByteArray,  uint8_t>  (src); break;
    case NDInt16:   fromValue<PVShortArray,  int16_t>  (src); break;
    case NDUInt16:  fromValue<PVUShortArray, uint16_t> (src); break;
    case NDInt32:   fromValue<PVIntArray,    int32_t>  (src); break;
    case NDUInt32:  fromValue<PVUIntArray,   uint32_t> (src); break;
    case NDFloat32: fromValue<PVFloatArray,  float>    (src); break;
    case NDFloat64: fromValue<PVDoubleArray, double>   (src); break;
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
        if (!destVec[i])
            destVec[i] = PVDC->createPVStructure(dimStructure);

        destVec[i]->getSubField<PVInt>("size")->put(src->dims[i].size);
        destVec[i]->getSubField<PVInt>("offset")->put(src->dims[i].offset);
        destVec[i]->getSubField<PVInt>("fullSize")->put(src->dims[i].size);
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

    TimeStamp ts(src->epicsTS.secPastEpoch, src->epicsTS.nsec);
    pvDest.set(ts);
}

template <typename pvAttrType, typename valueType>
void NTNDArrayConverter::fromAttribute (PVStructurePtr dest, NDAttribute *src)
{
    valueType value;
    src->getValue(src->getDataType(), (void*)&value);

    PVUnionPtr valueFieldUnion = dest->getSubField<PVUnion>("value");
    static_pointer_cast<pvAttrType>(valueFieldUnion->get())->put(value);
}

void NTNDArrayConverter::fromStringAttribute (PVStructurePtr dest,
        NDAttribute *src)
{
    NDAttrDataType_t attrDataType;
    size_t attrDataSize;

    src->getValueInfo(&attrDataType, &attrDataSize);

    char value[attrDataSize];
    src->getValue(attrDataType, value, attrDataSize);

    PVUnionPtr valueFieldUnion = dest->getSubField<PVUnion>("value");
    static_pointer_cast<PVString>(valueFieldUnion->get())->put(value);
}

void NTNDArrayConverter::createAttributes (NDArray *src)
{
    NDAttributeList *srcList = src->pAttributeList;
    NDAttribute *attr = srcList->next(NULL);
    PVStructureArrayPtr dest(m_array->getAttribute());
    PVStructureArray::svector destVec(dest->reuse());
    StructureConstPtr structure(dest->getStructureArray()->getStructure());

    while(attr)
    {
        PVStructurePtr pvAttr(PVDC->createPVStructure(structure));

        pvAttr->getSubField<PVString>("name")->put(attr->getName());
        pvAttr->getSubField<PVString>("descriptor")->put(attr->getDescription());
        pvAttr->getSubField<PVString>("source")->put(attr->getSource());

        NDAttrSource_t sourceType;
        attr->getSourceInfo(&sourceType);
        pvAttr->getSubField<PVInt>("sourceType")->put(sourceType);

        PVUnionPtr value = pvAttr->getSubField<PVUnion>("value");
        switch(attr->getDataType())
        {

        case NDAttrInt8:    value->set(PVDC->createPVScalar<PVByte>());   break;
        case NDAttrUInt8:   value->set(PVDC->createPVScalar<PVUByte>());  break;
        case NDAttrInt16:   value->set(PVDC->createPVScalar<PVShort>());  break;
        case NDAttrUInt16:  value->set(PVDC->createPVScalar<PVUShort>()); break;
        case NDAttrInt32:   value->set(PVDC->createPVScalar<PVInt>());    break;
        case NDAttrUInt32:  value->set(PVDC->createPVScalar<PVUInt>());   break;
        case NDAttrFloat32: value->set(PVDC->createPVScalar<PVFloat>());  break;
        case NDAttrFloat64: value->set(PVDC->createPVScalar<PVDouble>()); break;
        case NDAttrString:  value->set(PVDC->createPVScalar<PVString>()); break;
        case NDAttrUndefined:
        default:
            throw std::runtime_error("invalid attribute data type");
        }

        destVec.push_back(pvAttr);
        attr = srcList->next(attr);
    }

    m_array->getAttribute()->replace(freeze(destVec));
}

void NTNDArrayConverter::fromAttributes (NDArray *src)
{
    PVStructureArrayPtr dest(m_array->getAttribute());
    NDAttributeList *srcList = src->pAttributeList;

    if(dest->view().dataCount() != (size_t)srcList->count())
        createAttributes (src);

    PVStructureArray::svector destVec(dest->reuse());
    NDAttribute *attr = srcList->next(NULL);

    for(PVStructureArray::svector::iterator it = destVec.begin();
            it != destVec.end(); ++it)
    {
        PVStructurePtr pvAttr(*it);
        switch(attr->getDataType())
        {
        case NDAttrInt8:    fromAttribute <PVByte,   int8_t>  (pvAttr, attr); break;
        case NDAttrUInt8:   fromAttribute <PVUByte,  uint8_t> (pvAttr, attr); break;
        case NDAttrInt16:   fromAttribute <PVShort,  int16_t> (pvAttr, attr); break;
        case NDAttrUInt16:  fromAttribute <PVUShort, uint16_t>(pvAttr, attr); break;
        case NDAttrInt32:   fromAttribute <PVInt,    int32_t> (pvAttr, attr); break;
        case NDAttrUInt32:  fromAttribute <PVUInt,   uint32_t>(pvAttr, attr); break;
        case NDAttrFloat32: fromAttribute <PVFloat,  float>   (pvAttr, attr); break;
        case NDAttrFloat64: fromAttribute <PVDouble, double>  (pvAttr, attr); break;
        case NDAttrString:  fromStringAttribute(pvAttr, attr); break;
        case NDAttrUndefined:
        default:
            throw std::runtime_error("invalid attribute data type");
        }
        attr = srcList->next(attr);
    }

    dest->replace(freeze(destVec));
}




