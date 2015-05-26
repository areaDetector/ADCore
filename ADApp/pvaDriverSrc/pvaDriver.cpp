/* pvaDriver.cpp
 *
 * This is a driver for EPICSv4 pvAccess NTNDArrays.
 *
 * Author: Bruno Martins
 *         Brookhaven National Laboratory
 *
 * Created:  April 10, 2015
 *
 */
#include <epicsExport.h>
#include <epicsThread.h>
#include <iocsh.h>

#include <pv/clientFactory.h>
#include <pv/pvAccess.h>
#include <pv/ntndarray.h>

#include <ntndArrayConverter.h>

#include <ADDriver.h>
#include "pvaDriver.h"

//#define DEFAULT_REQUEST "record[queueSize=100]field()"
#define DEFAULT_REQUEST "field()"

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::nt;

static const char *driverName = "pvaDriver";

/* Constructor for pvaDriver; most parameters are simply passed to
 * ADDriver::ADDriver. Sets reasonable default values for parameters defined in
 * asynNDArrayDriver and ADDriver.
 *
 * The method init must be called after creating an instance.
 *
 * \param[in] portName The name of the asyn port driver to be created.
 * \param[in] pvName The v4 NTNDArray PV to be monitored
 * \param[in] maxBuffers The maximum number of NDArray buffers that the
 *            NDArrayPool for this driver is allowed to allocate. Set this to -1
 *            to allow an unlimited number of buffers.
 * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for
 *            this driver is allowed to allocate. Set this to -1 to allow an
 *            unlimited amount of memory.
 * \param[in] priority The thread priority for the asyn port driver thread if
 *            ASYN_CANBLOCK is set in asynFlags.
 * \param[in] stackSize The stack size for the asyn port driver thread if
 *            ASYN_CANBLOCK is set in asynFlags.
 */
pvaDriver::pvaDriver (const char *portName, const char *pvName,
        int maxBuffers, size_t maxMemory, int priority, int stackSize)

    : ADDriver(portName, 1, 3, maxBuffers, maxMemory, 0, 0, ASYN_CANBLOCK, 1,
            priority, stackSize),
      m_pvName(pvName), m_request(DEFAULT_REQUEST),
      m_priority(ChannelProvider::PRIORITY_DEFAULT),
      m_thisPtr(tr1::shared_ptr<pvaDriver>(this))
{
    int status = asynSuccess;
    const char *functionName = "pvaDriver";

    createParam(PVAOverrunCounterString,     asynParamInt32, &PVAOverrunCounter);
    createParam(PVAPvNameString,             asynParamOctet, &PVAPvName);
    createParam(PVAPvConnectionStatusString, asynParamInt32, &PVAPvConnectionStatus);

    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, "PVAccess driver");
    status |= setStringParam (ADModel, "Basic PVAccess driver");
    status |= setIntegerParam(ADMaxSizeX, 0);
    status |= setIntegerParam(ADMaxSizeY, 0);
    status |= setIntegerParam(ADMinX, 0);
    status |= setIntegerParam(ADMinY, 0);
    status |= setIntegerParam(ADBinX, 1);
    status |= setIntegerParam(ADBinY, 1);
    status |= setIntegerParam(ADReverseX, 0);
    status |= setIntegerParam(ADReverseY, 0);
    status |= setIntegerParam(ADSizeX, 0);
    status |= setIntegerParam(ADSizeY, 0);
    status |= setIntegerParam(NDArraySizeX, 0);
    status |= setIntegerParam(NDArraySizeY, 0);
    status |= setIntegerParam(NDArraySize, 0);
    status |= setIntegerParam(NDDataType, 0);
    status |= setIntegerParam(PVAOverrunCounter, 0);
    status |= setStringParam (PVAPvName, pvName);
    status |= setIntegerParam(PVAPvConnectionStatus, 0);

    if(status)
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s unable to set driver parameters\n",
                driverName, functionName);

    try
    {
        ClientFactory::start();
        m_provider = getChannelProviderRegistry()->getProvider("pva");
        m_channel = m_provider->createChannel(m_pvName, m_thisPtr, m_priority);
        m_pvRequest = CreateRequest::create()->createRequest(m_request);
        m_monitor = m_channel->createMonitor(m_thisPtr, m_pvRequest);
    }
    catch (exception &ex)
    {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s exception initializing monitor: %s\n",
                driverName, functionName, ex.what());
    }
}

string pvaDriver::getRequesterName (void)
{
    return string(driverName);
}

void pvaDriver::message (string const & message, MessageType messageType)
{
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s: [type=%s] %s\n",
            driverName, "message", getMessageTypeName(messageType).c_str(),
            message.c_str());
}

void pvaDriver::channelCreated (const Status& status,
        ChannelPtr const & channel)
{
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s: %s created\n",
            driverName, "channelCreated", channel->getChannelName().c_str());
}

void pvaDriver::channelStateChange (ChannelPtr const & channel,
        Channel::ConnectionState state)
{
    const char *functionName = "channelStateChange";

    lock();
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s %s: %s\n",
            driverName, functionName, channel->getChannelName().c_str(),
            Channel::ConnectionStateNames[state]);
    setIntegerParam(PVAPvConnectionStatus, state == Channel::CONNECTED);
    callParamCallbacks();
    unlock();
}

void pvaDriver::unlisten (MonitorPtr const & monitor)
{
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s monitor unlistens\n",
            driverName, "unlisten");
}

void pvaDriver::monitorConnect (Status const & status,
        MonitorPtr const & monitor, StructureConstPtr const & structure)
{
    const char *functionName = "monitorConnect";

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s monitor connects [type=%s]\n",
            driverName, functionName, Status::StatusTypeName[status.getType()]);

    if (status.isSuccess())
    {
        PVDataCreatePtr PVDC = getPVDataCreate();

        if(!NTNDArray::isCompatible(PVDC->createPVStructure(structure)))
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s incompatible PVStructure. Not starting monitor\n",
                    driverName, functionName);
            return;
        }

        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s starting monitor\n",
                driverName, functionName);
        monitor->start();
    }
}

void pvaDriver::monitorEvent (MonitorPtr const & monitor)
{
    const char *functionName = "monitorEvent";
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s Event!\n",
            driverName, functionName);
    lock();
    MonitorElementPtr update;
    while ((update = monitor->poll()))
    {
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                 "%s::%s update!\n",
                 driverName, functionName);

        if(!update->overrunBitSet->isEmpty())
        {
            int overrunCounter;
            getIntegerParam(PVAOverrunCounter, &overrunCounter);
            setIntegerParam(PVAOverrunCounter, overrunCounter + 1);
            callParamCallbacks();
        }

        NTNDArrayConverter converter(NTNDArray::wrap(update->pvStructurePtr));
        NTNDArrayInfo_t info;

        try
        {
            info = converter.getInfo();
        }
        catch(...)
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s failed to get info from NTNDArray\n",
                    driverName, functionName);
            monitor->release(update);
            continue;
        }

        NDArray *pImage = pNDArrayPool->alloc(info.ndims, (size_t*) &info.dims,
                info.dataType, info.totalBytes, NULL);

        if(!pImage)
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s failed to alloc new NDArray"
                    " - memory pool exhausted? (free: %d)\n",
                    driverName, functionName, pNDArrayPool->numFree());
            monitor->release(update);
            continue;
        }

        unlock();
        try
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                      "%s::%s Converting to NDArray\n",
                      driverName, functionName);
            converter.toArray(pImage);
        }
        catch(...)
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s failed to convert NTNDArray into NDArray\n",
                    driverName, functionName);
            pImage->release();
            monitor->release(update);
            lock();
            continue;
        }
        lock();

        int xSize     = pImage->dims[info.x.dim].size;
        int ySize     = pImage->dims[info.y.dim].size;
        setIntegerParam(ADMaxSizeX,   xSize);
        setIntegerParam(ADMaxSizeY,   ySize);
        setIntegerParam(ADSizeX,      xSize);
        setIntegerParam(ADSizeY,      ySize);
        setIntegerParam(NDArraySizeX, xSize);
        setIntegerParam(NDArraySizeY, ySize);
        setIntegerParam(NDArraySizeZ, pImage->dims[info.color.dim].size);
        setIntegerParam(ADMinX,       pImage->dims[info.x.dim].offset);
        setIntegerParam(ADMinY,       pImage->dims[info.y.dim].offset);
        setIntegerParam(ADBinX,       pImage->dims[info.x.dim].binning);
        setIntegerParam(ADBinY,       pImage->dims[info.y.dim].binning);
        setIntegerParam(ADReverseX,   pImage->dims[info.x.dim].reverse);
        setIntegerParam(ADReverseY,   pImage->dims[info.y.dim].reverse);
        setIntegerParam(NDArraySize,  (int) info.totalBytes);
        setIntegerParam(NDDataType,   (int) info.dataType);
        setIntegerParam(NDColorMode,  (int) info.colorMode);
        callParamCallbacks();

        int arrayCallbacks;
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        if(arrayCallbacks)
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                      "%s::%s Callback with NDArray (%p)\n",
                      driverName, functionName, pImage);
            unlock();
            doCallbacksGenericPointer(pImage, NDArrayData, 0);
            lock();
        }

        // Update the counters after doCallbacksGenericPointer()
        int imageCounter;
        getIntegerParam(NDArrayCounter, &imageCounter);
        setIntegerParam(NDArrayCounter, imageCounter+1);
        getIntegerParam(ADNumImagesCounter, &imageCounter);
        setIntegerParam(ADNumImagesCounter, imageCounter+1);
        callParamCallbacks();

        pImage->release();
        monitor->release(update);
    }
    unlock();
}

void pvaDriver::report (FILE *fp, int details)
{
    fprintf(fp, "PVAccess detector %s\n", this->portName);
    if (details > 0)
        fprintf(fp, " PV Name: %s\n", m_pvName.c_str());

    ADDriver::report(fp, details);
}

/** Configuration command, called directly or from iocsh */
extern "C" int pvaDriverConfig (const char *portName, char *pvName,
        int maxBuffers, int maxMemory, int priority, int stackSize)
{
    new pvaDriver(portName, pvName, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg pvaDriverConfigArg0 = {"Port name", iocshArgString};
static const iocshArg pvaDriverConfigArg1 = {"PV name", iocshArgString};
static const iocshArg pvaDriverConfigArg2 = {"maxBuffers", iocshArgInt};
static const iocshArg pvaDriverConfigArg3 = {"maxMemory", iocshArgInt};
static const iocshArg pvaDriverConfigArg4 = {"priority", iocshArgInt};
static const iocshArg pvaDriverConfigArg5 = {"stackSize", iocshArgInt};
static const iocshArg * const pvaDriverConfigArgs[] = {
        &pvaDriverConfigArg0, &pvaDriverConfigArg1, &pvaDriverConfigArg2,
        &pvaDriverConfigArg3, &pvaDriverConfigArg4, &pvaDriverConfigArg5};

static const iocshFuncDef configpvaDriver = {"pvaDriverConfig", 6,
        pvaDriverConfigArgs};

static void configpvaDriverCallFunc (const iocshArgBuf *args)
{
    pvaDriverConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival);
}

static void pvaDriverRegister (void)
{
    iocshRegister(&configpvaDriver, configpvaDriverCallFunc);
}

extern "C" {
    epicsExportRegistrar(pvaDriverRegister);
}
