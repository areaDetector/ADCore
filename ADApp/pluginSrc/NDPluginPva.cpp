#include <pv/pvDatabase.h>
#include <pv/nt.h>
#include <pv/channelProviderLocal.h>

#include <epicsThread.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <ntndArrayConverter.h>

#include <asynDriver.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "NDPluginDriver.h"
#include "NDPluginPva.h"

using namespace epics;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace epics::nt;
using namespace std;

class epicsShareClass NTNDArrayRecord :
    public PVRecord
{

private:
    NTNDArrayRecord(string const & name, PVStructurePtr const & pvStructure)
    :PVRecord(name, pvStructure) {}

    NTNDArrayPtr m_ntndArray;
    NTNDArrayConverterPtr m_converter;

public:
    POINTER_DEFINITIONS(NTNDArrayRecord);

    virtual ~NTNDArrayRecord () {}
    static NTNDArrayRecordPtr create (string const & name);
    virtual bool init ();
    virtual void destroy ();
    virtual void process () {}
    void update (NDArray *pArray);
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
    m_ntndArray = NTNDArray::wrap(getPVStructure());
    m_converter.reset(new NTNDArrayConverter(m_ntndArray));
    return true;
}

void NTNDArrayRecord::destroy()
{
    PVRecord::destroy();
}

void NTNDArrayRecord::update(NDArray *pArray)
{
    lock();

    try
    {
        beginGroupPut();
        m_converter->fromArray(pArray);
        endGroupPut();
    }
    catch(...)
    {
        endGroupPut();
        unlock();
        throw;
    }
    unlock();
}

/** Callback function that is called by the NDArray driver with new NDArray
  * data.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginPva::processCallbacks(NDArray *pArray)
{
    NDPluginDriver::processCallbacks(pArray);   // Base class method

    this->unlock();             // Function called with the lock taken
    m_record->update(pArray);
    this->lock();               // Must return locked

    callParamCallbacks();
}

/** Constructor for NDPluginPva; all parameters are simply passed to
  * NDPluginDriver::NDPluginDriver.
  * This plugin cannot block (ASYN_CANBLOCK=0) and is not multi-device
  * (ASYN_MULTIDEVICE=0).
  * It has 1 parameter (1)
  * It allocates a maximum of 2 NDArray buffers for internal use.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this
  *            plugin can hold when NDPluginDriverBlockingCallbacks=0.
  *            Larger queues can decrease the number of dropped arrays, at the
  *            expense of more NDArray buffers being allocated from the
  *            underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the
  *            NDPluginDriverBlockingCallbacks flag. 0=callbacks are queued and
  *            executed by the callback thread; 1 callbacks execute in the
  *            thread of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of
  *            NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of
  *            NDArray callbacks.
  * \param[in] pvName Name of the PV that will be served by the EPICSv4 server.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for
  *            this driver is allowed to allocate. Set this to -1 to allow an
  *            unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if
  *            ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if
  *            ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginPva::NDPluginPva(const char *portName, int queueSize,
        int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr,
        const char *pvName, size_t maxMemory, int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
            NDArrayPort, NDArrayAddr, 1, 1, 2, maxMemory, 0, 0,
            0, 1, priority, stackSize),
            m_record(NTNDArrayRecord::create(pvName))
{
    createParam(NDPluginPvaPvNameString, asynParamOctet, &NDPluginPvaPvName);

    if(!m_record.get())
        throw runtime_error("failed to create NTNDArrayRecord");

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginPva");

    /* Set PvName */
    setStringParam(NDPluginPvaPvName, pvName);

    /* Try to connect to the NDArray port */
    connectToArrayPort();

    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();

    if(!master->addRecord(m_record))
        throw runtime_error("couldn't add record to master database");

    m_server = startPVAServer(PVACCESS_ALL_PROVIDERS, 0, true, true);
}

/* Configuration routine.  Called directly, or from the iocsh function */
extern "C" int NDPvaConfigure(const char *portName, int queueSize,
        int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr,
        const char *pvName, size_t maxMemory, int priority, int stackSize)
{
    NDPluginPva *pPlugin = new NDPluginPva(portName, queueSize, blockingCallbacks, NDArrayPort,
                                           NDArrayAddr, pvName, maxMemory, priority, stackSize);
    return pPlugin->start();
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
static const iocshFuncDef initFuncDef = {"NDPvaConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDPvaConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval,
            args[4].ival, args[5].sval, args[6].ival, args[7].ival,
            args[8].ival);
}

extern "C" void NDPvaRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDPvaRegister);
}
