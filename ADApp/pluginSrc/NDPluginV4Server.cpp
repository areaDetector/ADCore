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

template <typename arrayPtrType, typename arrayType, typename svectorType,
          typename srcDataType>

static void copyToNTNDArray(PVUnionPtr dest, void *src, size_t count,
                            const string unionSelection)
{
    arrayPtrType pvField = dest->select<arrayType>(unionSelection);

    svectorType temp(pvField->reuse());
    temp.resize(count);

    srcDataType *data = (srcDataType*)src;
    std::copy(data, data + count, temp.begin());
    pvField->replace(freeze(temp));

    dest->postPut();
}

class epicsShareClass ImageRecord :
    public PVRecord
{

private:
    ImageRecord(string const & name, PVStructurePtr const & pvStructure)
    :PVRecord(name, pvStructure) {}

    PVUnionPtr m_pvValue;
    PVStructureArrayPtr m_pvDimension;
    PVLongPtr m_pvCompressedSize;
    PVLongPtr m_pvUncompressedSize;
    PVAlarm m_pvAlarm;
    PVTimeStamp m_pvTimestamp;
    PVIntPtr m_pvUniqueId;
    PVIntPtr m_pvColorMode;

public:
    POINTER_DEFINITIONS(ImageRecord);

    static ImageRecordPtr create (string const & name)
    {
        NTNDArrayBuilderPtr builder = NTNDArray::createBuilder();
        builder->addDescriptor()->addTimeStamp()->addAlarm()->addDisplay();

        ImageRecordPtr pvRecord(new ImageRecord(name,
                builder->createPVStructure()));

        if(!pvRecord->init())
            pvRecord.reset();

        return pvRecord;
    }

    virtual ~ImageRecord () {}

    virtual void destroy ()
    {
        PVRecord::destroy();
    }

    virtual bool init ()
    {
        initPVRecord();
        PVStructurePtr pvStructure = getPVStructure();

#define GET_FIELD(field,fieldstr,type)\
        field = pvStructure->getSubField<type>(fieldstr);\
        if(!field)\
            return false

        GET_FIELD(m_pvValue,            "value",            PVUnion);
        GET_FIELD(m_pvDimension,        "dimension",        PVStructureArray);
        GET_FIELD(m_pvCompressedSize,   "compressedSize",   PVLong);
        GET_FIELD(m_pvUncompressedSize, "uncompressedSize", PVLong);
        GET_FIELD(m_pvUniqueId,         "uniqueId",         PVInt);

#undef GET_FIELD

        PVFieldPtr pvField;

#define ATTACH_FIELD(field, fieldstr)\
        pvField = pvStructure->getSubField(fieldstr);\
        if(!pvField || !field.attach(pvField))\
            return false

        ATTACH_FIELD(m_pvAlarm, "alarm");
        ATTACH_FIELD(m_pvTimestamp, "timeStamp");


#undef ATTACH_FIELD

        // Set codec name to empty

        PVStringPtr codecName = pvStructure->getSubField<PVString>("codec.name");
        if(!codecName)
            return false;
        codecName->put("");

        // Add color attribute

        PVStructureArrayPtr pvAttrs = pvStructure->getSubField<PVStructureArray>("attribute");
        if(!pvAttrs)
            return false;

        PVStructureArray::svector attrs(pvAttrs->reuse());
        PVStructurePtr attr = getPVDataCreate()->createPVStructure(pvAttrs->getStructureArray()->getStructure());

        m_pvColorMode = getPVDataCreate()->createPVScalar<PVInt>();

        attr->getSubField<PVUnion>("value")->set(m_pvColorMode);
        attr->getSubField<PVString>("name")->put("ColorMode");
        attr->getSubField<PVString>("descriptor")->put("Color mode");
        attr->getSubField<PVInt>("sourceType")->put(0);
        attr->getSubField<PVString>("source")->put("");

        attrs.push_back(attr);
        pvAttrs->replace(freeze(attrs));

        return true;
    }

    virtual void process ()
    {
        cout << "process" << endl;
    }

    void update(NDArray *pArray)
    {
        lock();

        NDArrayInfo_t arrayInfo;
        pArray->getInfo(&arrayInfo);

        try
        {
            beginGroupPut();
            switch(arrayInfo.bytesPerElement)
            {
            case 1:
                copyToNTNDArray<PVByteArrayPtr,PVByteArray,PVByteArray::svector,int8_t>
                (m_pvValue, pArray->pData, arrayInfo.nElements, "byteValue");
                break;
            case 2:
                copyToNTNDArray<PVShortArrayPtr,PVShortArray,PVShortArray::svector,int16_t>
                (m_pvValue, pArray->pData, arrayInfo.nElements, "shortValue");
                break;
            case 4:
                copyToNTNDArray<PVIntArrayPtr,PVIntArray,PVIntArray::svector,int32_t>
                (m_pvValue, pArray->pData, arrayInfo.nElements, "intValue");
                break;
            case 8:
                copyToNTNDArray<PVLongArrayPtr,PVLongArray,PVLongArray::svector,int64_t>
                (m_pvValue, pArray->pData, arrayInfo.nElements, "longValue");
                break;
            default:
                throw runtime_error("invalid bytesPerElement");
            }

            PVStructureArray::svector dimVector(m_pvDimension->reuse());
            dimVector.resize(pArray->ndims);
            for (int i = 0; i < pArray->ndims; i++)
            {
                PVStructurePtr d = dimVector[i];
                if (!d)
                    d = dimVector[i] = getPVDataCreate()->createPVStructure(m_pvDimension->getStructureArray()->getStructure());
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
            m_pvColorMode->put(arrayInfo.colorMode);

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
};

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
                   m_image(ImageRecord::create(pvName))
{
    fprintf(stderr, "DEBUG: NDPluginV4Server constructor for pvName %s\n", pvName);

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
