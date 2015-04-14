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

class PVAChannelRequester;
class pvaDriver;

typedef Channel::shared_pointer ChannelPtr;
typedef ChannelProvider::shared_pointer ChannelProviderPtr;
typedef tr1::shared_ptr<MonitorElement> MonitorElementPtr;
typedef tr1::shared_ptr<PVAChannelRequester> PVAChannelRequesterPtr;
typedef tr1::shared_ptr<pvaDriver> pvaDriverPtr;

static const char *driverName = "pvaDriver";

class PVARequester : public virtual Requester
{
private:
    asynUser *m_asynUser;

protected:
    const char *m_name;

public:
    PVARequester(const char *name, asynUser *user) :
        m_name(name), m_asynUser(user){}

    string getRequesterName (void)
    {
        return string(m_name);
    }

    void message(string const & message, MessageType messageType)
    {
        asynPrint(m_asynUser, ASYN_TRACE_FLOW,
                "%s::%s: [type=%s] %s\n",
                m_name, "message", getMessageTypeName(messageType).c_str(),
                message.c_str());
    }
};

class PVAChannelRequester : public virtual PVARequester,
        public virtual ChannelRequester
{
private:
    asynUser *m_asynUser;

public:
    PVAChannelRequester(asynUser *user) :
        PVARequester("PVAChannelRequester", user),
        m_asynUser(user)
    {}

    void channelCreated (const Status& status, ChannelPtr const & channel)
    {
        asynPrint(m_asynUser, ASYN_TRACE_FLOW,
                "%s::%s: %s created\n",
                m_name, "channelCreated", channel->getChannelName().c_str());
    }

    void channelStateChange (ChannelPtr const & channel,
            Channel::ConnectionState state)
    {
        asynPrint(m_asynUser,
                Channel::CONNECTED ? ASYN_TRACE_FLOW : ASYN_TRACE_ERROR,
                "%s::%s %s: %s\n",
                m_name, "channelStateChange", channel->getChannelName().c_str(),
                Channel::ConnectionStateNames[state]);
    }
};

class epicsShareClass pvaDriver : public ADDriver,
        public virtual PVARequester, public virtual MonitorRequester
{

public:
    pvaDriver(const char *portName, const char *pvName, int maxBuffers,
            size_t maxMemory, int priority, int stackSize);

    // Overriden from ADDriver:
    virtual void report(FILE *fp, int details);

private:
    string m_pvName;
    string m_request;
    short m_priority;
    ChannelProviderPtr m_provider;
    PVAChannelRequesterPtr m_requester;
    ChannelPtr m_channel;
    PVStructurePtr m_pvRequest;
    MonitorPtr m_monitor;
    NDArray *m_pImage;

    // Implemented for MonitorRequester
    void monitorConnect (Status const & status, MonitorPtr const & monitor,
            StructureConstPtr const & structure);
    void monitorEvent (MonitorPtr const & monitor);

    void unlisten (MonitorPtr const & monitor)
    {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s monitor unlistens\n",
                driverName, "unlisten");
    }
};

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
pvaDriver::pvaDriver(const char *portName, const char *pvName,
        int maxBuffers, size_t maxMemory, int priority, int stackSize)

    : ADDriver(portName, 1, 0, maxBuffers, maxMemory, 0, 0, ASYN_CANBLOCK, 1,
            priority, stackSize),
      PVARequester("pvaDriver", pasynUserSelf),
      m_pvName(pvName), m_request(DEFAULT_REQUEST),
      m_priority(ChannelProvider::PRIORITY_DEFAULT),
      m_requester(new PVAChannelRequester(pasynUserSelf)),
      m_pImage(pNDArrayPool->alloc(0, NULL, NDInt8, 0, NULL))
{
    int status = asynSuccess;
    const char *functionName = "pvaDriver";
    pvaDriverPtr monitorRequester(this);

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

    if(status)
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s unable to set driver parameters\n",
                driverName, functionName);

    try
    {
        ClientFactory::start();
        m_provider = getChannelProviderRegistry()->getProvider("pva");

        if (!m_provider)
        {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s failed to stop monitor\n",
                    driverName, functionName);
            return;
        }

        m_channel = m_provider->createChannel(m_pvName, m_requester,m_priority);
        m_pvRequest = CreateRequest::create()->createRequest(m_request);
        m_monitor = m_channel->createMonitor(monitorRequester, m_pvRequest);
    }
    catch (exception &ex)
    {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s exception initializing monitor: %s\n",
                driverName, functionName, ex.what());
    }
}

void pvaDriver::monitorConnect(Status const & status,
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

        monitor->start();
    }
}

void pvaDriver::monitorEvent(MonitorPtr const & monitor)
{
    lock();

    MonitorElementPtr update;
    while ((update = monitor->poll()))
    {
        NTNDArrayConverter converter(NTNDArray::wrap(update->pvStructurePtr));
        converter.toArray(m_pImage);

        int imageCounter;
        getIntegerParam(NDArrayCounter, &imageCounter);
        imageCounter++;
        setIntegerParam(NDArrayCounter, imageCounter);
        callParamCallbacks();

        int xSize    = m_pImage->dims[0].size;
        int ySize    = m_pImage->dims[1].size;
        int xOffset  = m_pImage->dims[0].offset;
        int yOffset  = m_pImage->dims[1].offset;
        int xBinning = m_pImage->dims[0].binning;
        int yBinning = m_pImage->dims[1].binning;
        int xReverse = m_pImage->dims[0].reverse;
        int yReverse = m_pImage->dims[1].reverse;
        int dataType = (int) m_pImage->dataType;

        setIntegerParam(ADMaxSizeX, xSize);
        setIntegerParam(ADMaxSizeY, ySize);
        setIntegerParam(ADMinX, xOffset);
        setIntegerParam(ADMinY, yOffset);
        setIntegerParam(ADBinX, xBinning);
        setIntegerParam(ADBinY, yBinning);
        setIntegerParam(ADReverseX, xReverse);
        setIntegerParam(ADReverseY, yReverse);
        setIntegerParam(ADSizeX, xSize);
        setIntegerParam(ADSizeY, ySize);
        setIntegerParam(NDArraySizeX, xSize);
        setIntegerParam(NDArraySizeY, ySize);
        setIntegerParam(NDArraySize, xSize*ySize);
        setIntegerParam(NDDataType, dataType);
        callParamCallbacks();

        unlock();
        doCallbacksGenericPointer(m_pImage, NDArrayData, 0);
        lock();
        monitor->release(update);
    }
    unlock();
}

void pvaDriver::report(FILE *fp, int details)
{
    fprintf(fp, "PVAccess detector %s\n", this->portName);
    if (details > 0)
        fprintf(fp, " PV Name: %s\n", m_pvName.c_str());

    ADDriver::report(fp, details);
}

/** Configuration command, called directly or from iocsh */
extern "C" int pvaDriverConfig(const char *portName, char *pvName,
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

static void configpvaDriverCallFunc(const iocshArgBuf *args)
{
    pvaDriverConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival);
}

static void pvaDriverRegister(void)
{
    iocshRegister(&configpvaDriver, configpvaDriverCallFunc);
}

extern "C" {
    epicsExportRegistrar(pvaDriverRegister);
}
