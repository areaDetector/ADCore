/*
 * NDPluginInventory.cpp
 *
 * Process-wide registry of NDPlugin instances, published as a single PVAccess
 * structure keyed by plugin type.  See NDPluginInventory.h for details.
 *
 * The PVA backend is selected at compile time: PVXS is preferred, falling back
 * to pvDatabase/pvData.  If neither WITH_PVXS nor WITH_PVA is defined the
 * registry still works but publish() is a no-op.
 */

#include <algorithm>

#include <epicsMutex.h>
#include <epicsGuard.h>
#include <initHooks.h>
#include <asynPortDriver.h>
#include <iocsh.h>

#include "NDPluginInventory.h"

#include <epicsExport.h>

#if defined(WITH_PVXS)
  #include <pvxs/data.h>
  #include <pvxs/sharedpv.h>
  #include <pvxs/server.h>
  #include <pvxs/iochooks.h>
#elif defined(WITH_PVA)
  #include <pv/pvData.h>
  #include <pv/pvDatabase.h>
  #include <pv/channelProviderLocal.h>
#endif

namespace {

using NDPluginInventory::RegisteredPlugin;

// Mutex to protect updates to the plugin registry list.
epicsMutex& mutex() {
    static epicsMutex m;
    return m;
}

// Vector storing all registered plugins.  Access to this vector must be protected by the mutex.
// Plugins are initially added with just port names at construction, and then their
// type and PV prefix are populated later via setter functions. At iocInit() this
// vector is restructured into a map keyed by plugin type and published as a single PVAccess structure.
std::vector<RegisteredPlugin>& registry() {
    static std::vector<RegisteredPlugin> r;
    return r;
}


/**
 * Search for a plugin in the registry by its port name.
 * 
 * \param[in] portName The asyn port name of the plugin to find.
 * \return An iterator to the plugin if found, or registry().end() if not found
 */
std::vector<RegisteredPlugin>::iterator findPlugin(const std::string& portName)
{
    std::vector<RegisteredPlugin>& regs = registry();
    std::vector<RegisteredPlugin>::iterator it = regs.begin();
    for (; it != regs.end(); ++it) {
        if (it->portName == portName) break;
    }
    return it;
}

/**
 * Convert a plugin type string into a valid structure field name.
 * 
 * \param[in] type The plugin type string to convert.
 * \return A valid structure field name derived from the plugin type string.
 */
std::string toFieldName(const std::string& type)
{
    std::string name = type;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_';
        if (!ok) name[i] = '_';
    }
    if (name.empty() || (name[0] >= '0' && name[0] <= '9')) name = "_" + name;
    return name;
}

} // anonymous namespace

namespace NDPluginInventory {

/**
 * Register a plugin in the process-wide inventory.
 * 
 * \param[in] portName The asyn port name of the plugin to register.
 */
void registerPlugin(const std::string& portName)
{
    epicsGuard<epicsMutex> guard(mutex());
    if (findPlugin(portName) == registry().end()) {
        RegisteredPlugin plugin;
        plugin.portName = portName;
        registry().push_back(plugin);
    }
}

/**
 * Unregister a plugin from the process-wide inventory.
 * 
 * \param[in] portName The asyn port name of the plugin to unregister.
 */
void unregisterPlugin(const std::string& portName)
{
    epicsGuard<epicsMutex> guard(mutex());
    std::vector<RegisteredPlugin>::iterator it = findPlugin(portName);
    if (it != registry().end()) {
        registry().erase(it);
    }
}


/**
 * Set the plugin type for a registered plugin.
 * 
 * \param[in] portName The asyn port name of the plugin to update.
 * \param[in] pluginType The type of the plugin to set.
 */
void setPluginType(const std::string& portName, const std::string& pluginType)
{
    epicsGuard<epicsMutex> guard(mutex());
    std::vector<RegisteredPlugin>::iterator it = findPlugin(portName);
    if (it != registry().end()) {
        it->pluginType = pluginType;
    }
}

/**
 * Set the PV prefix for a registered plugin.
 * 
 * \param[in] portName The asyn port name of the plugin to update.
 * \param[in] pvPrefix The PV prefix to set for the plugin.
 */
void setPvPrefix(const std::string& portName, const std::string& pvPrefix)
{
    epicsGuard<epicsMutex> guard(mutex());
    std::vector<RegisteredPlugin>::iterator it = findPlugin(portName);
    if (it != registry().end()) {
        it->pvPrefix = pvPrefix;
    }
}

/**
 * Given the list of registered plugins, construct a mapping of plugin types to their corresponding plugins.
 * 
 * \return An inventory mapping plugin types to lists of registered plugins.
 */
Inventory getInventory()
{
    epicsGuard<epicsMutex> guard(mutex());
    Inventory inventory;
    std::vector<RegisteredPlugin>& regs = registry();
    for (std::vector<RegisteredPlugin>::const_iterator it = regs.begin(); it != regs.end(); ++it) {
        RegisteredPlugin plugin = *it;
        if (plugin.pluginType.empty()) plugin.pluginType = "Unknown";
        inventory[plugin.pluginType].push_back(plugin);
    }
    return inventory;
}

} // namespace NDPluginInventory

namespace {

#if defined(WITH_PVA) && !defined(WITH_PVXS)

class InventoryRecord;
typedef std::tr1::shared_ptr<InventoryRecord> InventoryRecordPtr;

// Minimal PVRecord wrapper around a pre-built PVStructure.
class InventoryRecord : public epics::pvDatabase::PVRecord {
public:
    static InventoryRecordPtr create(const std::string& name,
                                     epics::pvData::PVStructurePtr const & pvStructure)
    {
        InventoryRecordPtr rec(new InventoryRecord(name, pvStructure));
        if (!rec->init()) rec.reset();
        return rec;
    }
    virtual bool init() { initPVRecord(); return true; }
    virtual void process() {}
private:
    InventoryRecord(const std::string& name, epics::pvData::PVStructurePtr const & pvStructure)
        : epics::pvDatabase::PVRecord(name, pvStructure) {}
};

#endif

class PluginInventory : public asynPortDriver {
public:
    PluginInventory(const char *portName, const char *pvName)
        : asynPortDriver(portName, 1,
                         asynOctetMask | asynDrvUserMask, asynOctetMask,
                         0, 1, 0, 0),
          channelName_(pvName ? pvName : "")
    {
        createParam(NDPluginInventoryPvNameString, asynParamOctet, &pvName_);
        setStringParam(pvName_, channelName_.c_str());
#if defined(WITH_PVXS)
        pv_ = pvxs::server::SharedPV::buildReadonly();
        added_ = false;
#elif defined(WITH_PVA)
        // Register the pvDatabase "local" provider now, at config time, so the PVA
        // server includes it when it reads EPICS_PVAS_PROVIDER_NAMES during iocInit.
        // Without this the provider is first created at publish() (after iocInit) and
        // the server never serves the inventory channel.
        epics::pvDatabase::getChannelProviderLocal();
#endif
        theInstance_ = this;
        initHookRegister(&PluginInventory::initHook);
    }

    // Builds the inventory structure and (re)serves it on the configured channel.
    void publish()
    {
        if (channelName_.empty()) return;
        NDPluginInventory::Inventory inventory = NDPluginInventory::getInventory();
#if defined(WITH_PVXS) || defined(WITH_PVA)
        serve(inventory);
#else
        (void)inventory;
#endif
    }

private:
    static void initHook(initHookState state)
    {
        if (state == initHookAfterIocRunning && theInstance_) {
            theInstance_->publish();
        }
    }

#if defined(WITH_PVXS)
    void serve(const NDPluginInventory::Inventory& inventory)
    {
        using NDPluginInventory::RegisteredPlugin;

        // One member per plugin type, each a struct array of {portName, pvPrefix}.
        std::vector<pvxs::Member> children;
        for (NDPluginInventory::Inventory::const_iterator it = inventory.begin();
             it != inventory.end(); ++it) {
            std::vector<pvxs::Member> row;
            row.push_back(pvxs::members::String("portName"));
            row.push_back(pvxs::members::String("pvPrefix"));
            children.push_back(pvxs::members::StructA(toFieldName(it->first), row));
        }

        pvxs::TypeDef def(pvxs::TypeCode::Struct, "epics:nt/NTTable:1.0", children);
        pvxs::Value root = def.create();

        for (NDPluginInventory::Inventory::const_iterator it = inventory.begin();
             it != inventory.end(); ++it) {
            const std::string field = toFieldName(it->first);
            const std::vector<RegisteredPlugin>& plugins = it->second;
            pvxs::shared_array<pvxs::Value> arr(plugins.size());
            for (size_t i = 0; i < plugins.size(); ++i) {
                pvxs::Value elem = root[field].allocMember();
                elem["portName"] = plugins[i].portName;
                elem["pvPrefix"] = plugins[i].pvPrefix;
                arr[i] = elem;
            }
            root[field] = arr.freeze();
        }

        if (!added_) {
            pvxs::ioc::server().addPV(channelName_, pv_);
            added_ = true;
        }
        if (pv_.isOpen()) {
            pv_.close();
        }
        pv_.open(root);
    }
#elif defined(WITH_PVA)
    void serve(const NDPluginInventory::Inventory& inventory)
    {
        using namespace epics::pvData;
        using NDPluginInventory::RegisteredPlugin;

        FieldBuilderPtr fb = getFieldCreate()->createFieldBuilder();
        for (NDPluginInventory::Inventory::const_iterator it = inventory.begin();
             it != inventory.end(); ++it) {
            fb->addNestedStructureArray(toFieldName(it->first))
                  ->add("portName", pvString)
                  ->add("pvPrefix", pvString)
              ->endNested();
        }
        StructureConstPtr structure = fb->createStructure();
        PVStructurePtr pvStructure = getPVDataCreate()->createPVStructure(structure);

        for (NDPluginInventory::Inventory::const_iterator it = inventory.begin();
             it != inventory.end(); ++it) {
            const std::vector<RegisteredPlugin>& plugins = it->second;
            PVStructureArrayPtr array = pvStructure->getSubField<PVStructureArray>(toFieldName(it->first));
            StructureConstPtr elemType = array->getStructureArray()->getStructure();
            PVStructureArray::svector vec(plugins.size());
            for (size_t i = 0; i < plugins.size(); ++i) {
                PVStructurePtr elem = getPVDataCreate()->createPVStructure(elemType);
                elem->getSubField<PVString>("portName")->put(plugins[i].portName);
                elem->getSubField<PVString>("pvPrefix")->put(plugins[i].pvPrefix);
                vec[i] = elem;
            }
            array->replace(freeze(vec));
        }

        epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
        epics::pvDatabase::getChannelProviderLocal();
        if (master->findRecord(channelName_)) {
            master->removeRecord(master->findRecord(channelName_));
        }
        InventoryRecordPtr record = InventoryRecord::create(channelName_, pvStructure);
        if (record) {
            master->addRecord(record);
        }
    }
#endif

    int pvName_;
    std::string channelName_;
#if defined(WITH_PVXS)
    pvxs::server::SharedPV pv_;
    bool added_;
#endif
    static PluginInventory* theInstance_;
};

PluginInventory* PluginInventory::theInstance_ = NULL;

} // anonymous namespace


// IOC shell registration for the plugin inventory.
extern "C" int NDPluginInventoryConfigure(const char *portName, const char *pvName)
{
    new PluginInventory(portName, pvName);
    return asynSuccess;
}

static const iocshArg initArg0 = { "portName", iocshArgString };
static const iocshArg initArg1 = { "pvName", iocshArgString };
static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };
static const iocshFuncDef initFuncDef = { "NDPluginInventoryConfigure", 2, initArgs };
static void initCallFunc(const iocshArgBuf *args)
{
    NDPluginInventoryConfigure(args[0].sval, args[1].sval);
}

extern "C" void NDPluginInventoryRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDPluginInventoryRegister);
}
