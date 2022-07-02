#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pv/pvAccess.h>
#include <pva/server.h>
#include <pva/sharedstate.h>
#include <pv/nt.h>

#include <iocsh.h>

#include <ntndArrayConverter.h>

#include "NDPluginPva.h"

#include <epicsExport.h>

static const char *driverName="NDPluginPva";

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;
namespace nt = epics::nt;

namespace {
/* pvDatabase compatibility
 * Sends all fields which are assigned by NTNDArrayConverter,
 * regardless of whether they change.
 */
struct BitMarker : public pvd::PostHandler {
    const pvd::PVFieldPtr fld;
    const pvd::BitSetPtr mask;

    BitMarker(const pvd::PVFieldPtr& fld,
              const pvd::BitSetPtr& mask)
        :fld(fld)
        ,mask(mask)
    {}
    virtual ~BitMarker() {}

    virtual void postPut() OVERRIDE FINAL
    {
        mask->set(fld->getFieldOffset());
    }
};
}

class NTNDArrayRecord
{
public:
    pvas::StaticProvider provider;
    pvas::SharedPV::shared_pointer pv;

    const pvd::PVStructurePtr current;
    // wraps 'current'
    const nt::NTNDArrayPtr m_ntndArray;
    NTNDArrayConverter m_converter;

    pvd::BitSetPtr changes;

    NTNDArrayRecord(const std::string& name)
        :provider(pvas::StaticProvider(name))
        ,current(nt::NTNDArray::createBuilder()
                 ->addDescriptor()->addTimeStamp()->addAlarm()->addDisplay()
                 ->createPVStructure())
        ,m_ntndArray(nt::NTNDArray::wrap(current))
        ,m_converter(m_ntndArray)
        ,changes(new pvd::BitSet(current->getNumberFields()))
    {
        pvas::SharedPV::Config pv_conf;
        // pvDatabase compatibility mode for pvRequest handling
        pv_conf.mapperMode = pvd::PVRequestMapper::Slice;

        pv = pvas::SharedPV::buildReadOnly(&pv_conf);
        pv->open(*current);
        provider.add(name, pv);

        {
            std::tr1::shared_ptr<BitMarker> temp(new BitMarker(current, changes));
            current->setPostHandler(temp);
        }
        for(size_t i=current->getFieldOffset()+1u, N=current->getNextFieldOffset(); i<N; i++) {
            pvd::PVFieldPtr fld(current->getSubFieldT(i));
            std::tr1::shared_ptr<BitMarker> temp(new BitMarker(fld, changes));
            fld->setPostHandler(temp);
        }
    }

    void update(NDArray *pArray)
    {
        changes->clear();
        // through several levels of indirection, updates this->current
        // and through several more, updates this->changes
        m_converter.fromArray(pArray);

        // SharedPV::post() makes a lightweight copy of *current,
        // so we may safely continue to change it.

        pv->post(*current, *changes);
    }
};

/** Callback function that is called by the NDArray driver with new NDArray
  * data.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginPva::processCallbacks(NDArray *pArray)
{
    static const char *functionName = "processCallbacks";

    NDPluginDriver::beginProcessCallbacks(pArray);   // Base class method

    // Most plugins can rely on endProcessCallbacks() to check for throttling, but this one cannot
    // because the output is not an NDArray but a pvAccess server.  Need to check here.
    if (throttled(pArray)) {
        int droppedOutputArrays;
        int arrayCounter;
        getIntegerParam(NDPluginDriverDroppedOutputArrays, &droppedOutputArrays);
        asynPrint(pasynUserSelf, ASYN_TRACE_WARNING,
            "%s::%s maximum byte rate exceeded, dropped array uniqueId=%d\n",
            driverName, functionName, pArray->uniqueId);
        droppedOutputArrays++;
        setIntegerParam(NDPluginDriverDroppedOutputArrays, droppedOutputArrays);
        // Since this plugin has done no useful work we also decrement ArrayCounter
        getIntegerParam(NDArrayCounter, &arrayCounter);
        arrayCounter--;
        setIntegerParam(NDArrayCounter, arrayCounter);
    } else {
        this->unlock();             // Function called with the lock taken
        m_record->update(pArray);
        this->lock();               // Must return locked
    }

    // Do NDArray callbacks.  We need to copy the array and get the attributes
    NDPluginDriver::endProcessCallbacks(pArray, true, true);

    callParamCallbacks();
}

/** Constructor for NDPluginPva
  * This plugin cannot block (ASYN_CANBLOCK=0) and is not multi-device (ASYN_MULTIDEVICE=0).
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
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to 0 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to 0 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  *            This value should also be used for any other threads this object creates.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  *            This value should also be used for any other threads this object creates.
  */
NDPluginPva::NDPluginPva(const char *portName, int queueSize,
        int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr,
        const char *pvName, int maxBuffers, size_t maxMemory, int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
            NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory, 0, 0,
            0, 1, priority, stackSize, 1, true),
            m_record(new NTNDArrayRecord(pvName))
{
    createParam(NDPluginPvaPvNameString, asynParamOctet, &NDPluginPvaPvName);

    if(!m_record.get())
        throw std::runtime_error("failed to create NTNDArrayRecord");

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginPva");

    /* Set PvName */
    setStringParam(NDPluginPvaPvName, pvName);

    /* Try to connect to the NDArray port */
    connectToArrayPort();

    pva::ChannelProviderRegistry::servers()
            ->addSingleton(m_record->provider.provider());
}

NDPluginPva::~NDPluginPva() {}

/* Configuration routine.  Called directly, or from the iocsh function */
extern "C" int NDPvaConfigure(const char *portName, int queueSize,
        int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr,
        const char *pvName, int maxBuffers, size_t maxMemory, int priority, int stackSize)
{
    NDPluginPva *pPlugin = new NDPluginPva(portName, queueSize, blockingCallbacks, NDArrayPort,
                                           NDArrayAddr, pvName, maxBuffers, maxMemory, priority, stackSize);
    return pPlugin->start();
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "pvName",iocshArgString};
static const iocshArg initArg6 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg7 = { "maxMemory",iocshArgInt};
static const iocshArg initArg8 = { "priority",iocshArgInt};
static const iocshArg initArg9 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9,};
static const iocshFuncDef initFuncDef = {"NDPvaConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDPvaConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].sval,
                   args[6].ival, args[7].ival, args[8].ival,
                   args[9].ival);
}

extern "C" void NDPvaRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDPvaRegister);
}
