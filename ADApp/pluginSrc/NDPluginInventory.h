#ifndef NDPluginInventory_H
#define NDPluginInventory_H

#include <string>
#include <vector>
#include <map>

#include <NDPluginAPI.h>

/** Process-wide registry of NDPlugin instances.
  *
  * Every NDPluginDriver registers itself here at construction; its plugin type
  * and EPICS PV prefix are filled in via setters as they become known.  The
  * collected information (asyn port name, EPICS PV prefix and plugin type) is
  * published once, when the IOC reaches the running state, as a single PVAccess
  * structure keyed by plugin type.  This lets clients discover which plugins exist
  * and with what EPICS PV prefix they are associated,
  * without any knowledge of the (site specific) IOC startup configuration.
  */
namespace NDPluginInventory {

/** asyn parameter string for the inventory port's PVA channel name. */
#define NDPluginInventoryPvNameString "PV_NAME"

/** Information about a single registered plugin instance. */
struct RegisteredPlugin {
    std::string portName;     /**< asyn port name, e.g. "ROI1" */
    std::string pluginType;   /**< plugin class name, e.g. "NDPluginROI" */
    std::string pvPrefix;     /**< EPICS record prefix $(P)$(R), e.g. "13SIM1:ROI1:" */
};

/** map of plugin type strings to lists of registered plugins of that type */
typedef std::map<std::string, std::vector<RegisteredPlugin>> Inventory;

NDPLUGIN_API void registerPlugin(const std::string& portName);
NDPLUGIN_API void unregisterPlugin(const std::string& portName);
NDPLUGIN_API void setPluginType(const std::string& portName, const std::string& pluginType);
NDPLUGIN_API void setPvPrefix(const std::string& portName, const std::string& pvPrefix);

/** Returns the current inventory grouped by plugin type. */
NDPLUGIN_API Inventory getInventory();

} // namespace NDPluginInventory

#endif
