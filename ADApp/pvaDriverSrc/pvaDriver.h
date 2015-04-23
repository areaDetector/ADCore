#include <pv/clientFactory.h>
#include <pv/pvAccess.h>
#include <pv/ntndarray.h>

#define PVAOverrunCounterString   "OVERRUN_COUNTER"

class PVAChannelRequester;
class pvaDriver;

typedef epics::pvAccess::Channel::shared_pointer ChannelPtr;
typedef epics::pvAccess::ChannelProvider::shared_pointer ChannelProviderPtr;
typedef std::tr1::shared_ptr<epics::pvData::MonitorElement> MonitorElementPtr;
typedef std::tr1::shared_ptr<PVAChannelRequester> PVAChannelRequesterPtr;
typedef std::tr1::shared_ptr<pvaDriver> pvaDriverPtr;

class PVARequester : public virtual epics::pvData::Requester
{
private:
    asynUser *m_asynUser;

protected:
    const char *m_name;

public:
    PVARequester(const char *name, asynUser *user);

    std::string getRequesterName (void);
    void message(std::string const & message,
            epics::pvData::MessageType messageType);
};

class PVAChannelRequester : public virtual PVARequester,
        public virtual epics::pvAccess::ChannelRequester
{
private:
    asynUser *m_asynUser;

public:
    PVAChannelRequester(asynUser *user);

    void channelCreated (const epics::pvData::Status& status,
            ChannelPtr const & channel);

    void channelStateChange (ChannelPtr const & channel,
            epics::pvAccess::Channel::ConnectionState state);
};

class epicsShareClass pvaDriver : public ADDriver,
        public virtual PVARequester,
        public virtual epics::pvData::MonitorRequester
{

public:
    pvaDriver(const char *portName, const char *pvName, int maxBuffers,
            size_t maxMemory, int priority, int stackSize);

    // Overriden from ADDriver:
    virtual void report(FILE *fp, int details);

protected:
    int PVAOverrunCounter;

private:
    std::string m_pvName;
    std::string m_request;
    short m_priority;
    ChannelProviderPtr m_provider;
    PVAChannelRequesterPtr m_requester;
    ChannelPtr m_channel;
    epics::pvData::PVStructurePtr m_pvRequest;
    epics::pvData::MonitorPtr m_monitor;

    // Implemented for MonitorRequester
    void monitorConnect (epics::pvData::Status const & status,
            epics::pvData::MonitorPtr const & monitor,
            epics::pvData::StructureConstPtr const & structure);
    void monitorEvent (epics::pvData::MonitorPtr const & monitor);
    void unlisten (epics::pvData::MonitorPtr const & monitor);
};
