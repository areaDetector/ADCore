#include <pv/pvDatabase.h>
#include <pv/nt.h>
#include <pv/channelProviderLocal.h>

#include <epicsThread.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <stdlib.h>
#include <vector>
#include <string.h>
#include <stdio.h>

#include <math.h>

#include "NDPluginDriver.h"
#include "NDPluginV4Server.h"

using namespace epics;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace epics::nt;
using namespace std;
using tr1::static_pointer_cast;
using tr1::dynamic_pointer_cast;

template <typename arrayType, typename srcDataType>
static void copyToNTNDArray(PVUnionPtr dest, void *src, size_t count)
{
    string unionField = ScalarTypeFunc::name(arrayType::typeCode);
    unionField += "Value";

    tr1::shared_ptr<arrayType> pvField = dest->select<arrayType>(unionField);

    shared_vector<typename arrayType::value_type> temp(pvField->reuse());
    temp.resize(count);

    srcDataType *data = (srcDataType*)src;
    std::copy(data, data + count, temp.begin());
    pvField->replace(freeze(temp));

    dest->postPut();
}

template <typename pvAttrType, typename valueType>
static void updateValueField(PVStructurePtr pvAttr, NDAttribute *attr)
{
    valueType value;
    attr->getValue(attr->getDataType(), (void*)&value);

    PVUnionPtr valueFieldUnion = pvAttr->getSubField<PVUnion>("value");
    static_pointer_cast<pvAttrType>(valueFieldUnion->get())->put(value);
}

static PVDataCreatePtr PVDC = getPVDataCreate();

static void updateValueFieldString(PVStructurePtr pvAttr, NDAttribute *attr)
{
    NDAttrDataType_t attrDataType;
    size_t attrDataSize;

    attr->getValueInfo(&attrDataType, &attrDataSize);

    char value[attrDataSize];
    attr->getValue(attrDataType, value, attrDataSize);

    PVUnionPtr valueFieldUnion = pvAttr->getSubField<PVUnion>("value");
    static_pointer_cast<PVString>(valueFieldUnion->get())->put(value);
}

class epicsShareClass NTNDArrayRecord :
    public PVRecord
{

private:
    NTNDArrayRecord(string const & name, PVStructurePtr const & pvStructure)
    :PVRecord(name, pvStructure) {}

    PVUnionPtr m_pvValue;
    PVStructureArrayPtr m_pvDimension;
    PVLongPtr m_pvCompressedSize;
    PVLongPtr m_pvUncompressedSize;
    PVAlarm m_pvAlarm;
    PVTimeStamp m_pvTimestamp;
    PVTimeStamp m_pvDataTimeStamp;
    PVIntPtr m_pvUniqueId;
    PVStructureArrayPtr m_pvAttrs;

public:
    POINTER_DEFINITIONS(NTNDArrayRecord);

    virtual ~NTNDArrayRecord () {}
    static NTNDArrayRecordPtr create (string const & name);
    virtual bool init ();

    virtual void destroy ()
    {
        PVRecord::destroy();
    }

    virtual void process ()
    {
        cout << "process" << endl;
    }

    void update (NDArray *pArray);
    void createAttributes (NDAttributeList *attrs, PVStructureArray::svector& pvAttrsVector);
};

NTNDArrayRecordPtr NTNDArrayRecord::create (string const & name)
{
    NTNDArrayBuilderPtr builder = NTNDArray::createBuilder();
    builder->addDescriptor()->addTimeStamp()->addAlarm()->addDisplay();

    NTNDArrayRecordPtr pvRecord(new NTNDArrayRecord(name,
            builder->createPVStructure()));

    if(!pvRecord->init())
        pvRecord.reset();

    return pvRecord;
}

bool NTNDArrayRecord::init ()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    NTNDArrayPtr ntndArray = NTNDArray::wrap(pvStructure);

    m_pvValue            = ntndArray->getValue();
    m_pvDimension        = ntndArray->getDimension();
    m_pvCompressedSize   = ntndArray->getCompressedDataSize();
    m_pvUncompressedSize = ntndArray->getUncompressedDataSize();
    m_pvUniqueId         = ntndArray->getUniqueId();
    m_pvAttrs            = ntndArray->getAttribute();

    m_pvAlarm.attach(ntndArray->getAlarm());
    m_pvTimestamp.attach(ntndArray->getTimeStamp());
    m_pvDataTimeStamp.attach(ntndArray->getDataTimeStamp());

    // Set codec name to empty
    ntndArray->getCodec()->getSubField<PVString>("name")->put("");

    return true;
}

void NTNDArrayRecord::createAttributes (NDAttributeList *attrs, PVStructureArray::svector& pvAttrsVector)
{
    NDAttribute *attr = attrs->next(NULL);

    while(attr)
    {
        PVStructurePtr pvAttr;
        pvAttr = PVDC->createPVStructure(m_pvAttrs->getStructureArray()->getStructure());

        pvAttr->getSubField<PVString>("name")->put(attr->getName());
        pvAttr->getSubField<PVString>("descriptor")->put(attr->getDescription());
        pvAttr->getSubField<PVString>("source")->put(attr->getSource());

        NDAttrSource_t sourceType;
        attr->getSourceInfo(&sourceType);
        pvAttr->getSubField<PVInt>("sourceType")->put(sourceType);

        PVUnionPtr valueField = pvAttr->getSubField<PVUnion>("value");
        switch(attr->getDataType())
        {

        case NDAttrInt8:    valueField->set(PVDC->createPVScalar<PVByte>());   break;
        case NDAttrUInt8:   valueField->set(PVDC->createPVScalar<PVUByte>());  break;
        case NDAttrInt16:   valueField->set(PVDC->createPVScalar<PVShort>());  break;
        case NDAttrUInt16:  valueField->set(PVDC->createPVScalar<PVUShort>()); break;
        case NDAttrInt32:   valueField->set(PVDC->createPVScalar<PVInt>());    break;
        case NDAttrUInt32:  valueField->set(PVDC->createPVScalar<PVUInt>());   break;
        case NDAttrFloat32: valueField->set(PVDC->createPVScalar<PVFloat>());  break;
        case NDAttrFloat64: valueField->set(PVDC->createPVScalar<PVDouble>()); break;
        case NDAttrString:  valueField->set(PVDC->createPVScalar<PVString>()); break;
        case NDAttrUndefined:
        default:
            throw std::runtime_error("invalid attribute data type");
        }

        pvAttrsVector.push_back(pvAttr);
        attr = attrs->next(attr);
    }
}

void NTNDArrayRecord::update(NDArray *pArray)
{
    lock();

    NDArrayInfo_t arrayInfo;
    pArray->getInfo(&arrayInfo);

    try
    {
        beginGroupPut();
        switch(pArray->dataType)
        {
        case NDInt8:
            copyToNTNDArray<PVByteArray, int8_t>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDUInt8:
            copyToNTNDArray<PVUByteArray, uint8_t>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDInt16:
            copyToNTNDArray<PVShortArray, int16_t>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDUInt16:
            copyToNTNDArray<PVUShortArray, uint16_t>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDInt32:
            copyToNTNDArray<PVIntArray, int32_t>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDUInt32:
            copyToNTNDArray<PVUIntArray, uint32_t>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDFloat32:
            copyToNTNDArray<PVFloatArray, float>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        case NDFloat64:
            copyToNTNDArray<PVDoubleArray, double>
            (m_pvValue, pArray->pData, arrayInfo.nElements);
            break;
        }

        PVStructureArray::svector dimVector(m_pvDimension->reuse());
        dimVector.resize(pArray->ndims);
        for (int i = 0; i < pArray->ndims; i++)
        {
            PVStructurePtr d = dimVector[i];
            if (!d)
                d = dimVector[i] = PVDC->createPVStructure(m_pvDimension->getStructureArray()->getStructure());
            d->getSubField<PVInt>("size")->put(pArray->dims[i].size);
            d->getSubField<PVInt>("offset")->put(pArray->dims[i].offset);
            d->getSubField<PVInt>("fullSize")->put(pArray->dims[i].size);
            d->getSubField<PVInt>("binning")->put(pArray->dims[i].binning);
            d->getSubField<PVBoolean>("reverse")->put(pArray->dims[i].reverse);
        }
        m_pvDimension->replace(freeze(dimVector));

        m_pvCompressedSize->put(static_cast<int64>(pArray->dataSize));
        m_pvUncompressedSize->put(static_cast<int64>(pArray->dataSize));
        m_pvUniqueId->put(pArray->uniqueId);

        NDAttributeList *attrs = pArray->pAttributeList;
        PVStructureArray::svector pvAttrsVector(m_pvAttrs->reuse());

        if(!pvAttrsVector.dataCount() && attrs->count())
            createAttributes(attrs, pvAttrsVector);

        NDAttribute *attr = attrs->next(NULL);

        for(PVStructureArray::svector::iterator it = pvAttrsVector.begin();
                it != pvAttrsVector.end(); ++it)
        {
            PVStructurePtr pvAttr = *it;
            switch(attr->getDataType())
            {
            case NDAttrInt8:    updateValueField<PVByte,   int8_t>  (pvAttr, attr); break;
            case NDAttrUInt8:   updateValueField<PVUByte,  uint8_t> (pvAttr, attr); break;
            case NDAttrInt16:   updateValueField<PVShort,  int16_t> (pvAttr, attr); break;
            case NDAttrUInt16:  updateValueField<PVUShort, uint16_t>(pvAttr, attr); break;
            case NDAttrInt32:   updateValueField<PVInt,    int32_t> (pvAttr, attr); break;
            case NDAttrUInt32:  updateValueField<PVUInt,   uint32_t>(pvAttr, attr); break;
            case NDAttrFloat32: updateValueField<PVFloat,  float>   (pvAttr, attr); break;
            case NDAttrFloat64: updateValueField<PVDouble, double>  (pvAttr, attr); break;
            case NDAttrString:  updateValueFieldString(pvAttr, attr); break;
            case NDAttrUndefined:
            default:
                throw std::runtime_error("invalid attribute data type");
            }
            attr = attrs->next(attr);
        }

        m_pvAttrs->replace(freeze(pvAttrsVector));

        double seconds = floor(pArray->timeStamp);
        double nanoseconds = (pArray->timeStamp - seconds)*1e9;
        TimeStamp dataTs((int64_t)seconds, (int32)nanoseconds);
        m_pvDataTimeStamp.set(dataTs);

        TimeStamp ts(pArray->epicsTS.secPastEpoch, pArray->epicsTS.nsec);
        m_pvTimestamp.set(ts);
        endGroupPut();
    }
    catch(...)
    {
        unlock();
        throw;
    }
    unlock();
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginV4Server::processCallbacks(NDArray *pArray)
{
    NDPluginDriver::processCallbacks(pArray);   // Base class method

    this->unlock();             // Function called with the lock taken
    m_image->update(pArray);
    this->lock();               // Must return locked

    callParamCallbacks();
}

/** Constructor for NDPluginV4Server; all parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * This plugin cannot block (ASYN_CANBLOCK=0) and is not multi-device (ASYN_MULTIDEVICE=0).
  * It has no parameters (0)
  * It allocates a maximum of 2 NDArray buffers for internal use.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] pvName Name of the PV that will be served by the EPICSv4 server.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginV4Server::NDPluginV4Server(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr, const char *pvName,
                                     size_t maxMemory, int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, 0, 2, maxMemory, 0, 0,
                   /* asynFlags is set to 0, because this plugin cannot block and is not multi-device.
                    * It does autoconnect */
                   0, 1, priority, stackSize),
                   m_image(NTNDArrayRecord::create(pvName))
{
    if(!m_image.get())
        throw runtime_error("failed to create ImageRecord");

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginV4Server");

    /* Try to connect to the NDArray port */
    connectToArrayPort();

    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();

    if(!master->addRecord(m_image))
        throw runtime_error("couldn't add record to master database");

    m_server = startPVAServer(PVACCESS_ALL_PROVIDERS, 0, true, true);
}

/* Configuration routine.  Called directly, or from the iocsh function */
extern "C" int NDV4ServerConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                    const char *NDArrayPort, int NDArrayAddr, const char *pvName,
                                    size_t maxMemory, int priority, int stackSize)
{
    new NDPluginV4Server(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, pvName,
                          maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "pvName",iocshArgString};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,};
static const iocshFuncDef initFuncDef = {"NDV4ServerConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDV4ServerConfigure(args[0].sval, args[1].ival, args[2].ival,
                         args[3].sval, args[4].ival, args[5].sval,
                         args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDV4ServerRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDV4ServerRegister);
}
