#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <iocsh.h>
#include <initHooks.h>
#include <epicsMath.h>
#include <epicsThread.h>

#include <epicsExport.h>
#include "NDPluginAttrPlot.h"
#include "CircularBuffer.h"

ExposeDataTask::ExposeDataTask(NDPluginAttrPlot& plugin)
    : plugin_(plugin),
      stop_(false),
      thread_(*this, "dataExpose", epicsThreadGetStackSize(epicsThreadStackBig),
              epicsThreadPriorityMedium)
{ }

void ExposeDataTask::start() {
    thread_.start();
}

void ExposeDataTask::run() {
    while (!stop_) {
        plugin_.lock();
        plugin_.callback_data();
        plugin_.unlock();
        epicsThreadSleep(ND_ATTRPLOT_DATA_EXPOSURE_PERIOD);
    }
}

void ExposeDataTask::stop() {
    stop_ = true;
}

NDPluginAttrPlot::NDPluginAttrPlot(const char * port, int n_attributes,
        int cache_size, int n_data_blocks, const char * in_port, int in_addr,
        int queue_size, int blocking_callbacks, int priority, int stackSize)
    : NDPluginDriver (
                port,  // The name of the asyn port driver to be created
                queue_size, // The number of NDArrays that the input queue
                blocking_callbacks, // NDPluginDriverBlockingCallbacks flag
                in_port,     // Source of NDArray callbacks - port
                in_addr,     // Source of NDArray callbacks - address
                std::max(n_attributes, n_data_blocks), // Max number of addresses
                NUM_NDATTRPLOT_PARAMS, /* The number of parameters that
                                        * the class supports */
                -1,    // The maximum number of NDArray buffers (-1 = unlimited)
                -1,    // The maximum amount of memory (-1 = unlimited)
                asynFloat64ArrayMask | asynGenericPointerMask | asynInt32Mask |
                        asynFloat64Mask | asynOctetMask, /* Bit mask for asyn
                                    * interfaces that this driver supports */
                asynFloat64ArrayMask | asynGenericPointerMask | asynInt32Mask |
                        asynFloat64Mask | asynOctetMask, /* Bit mask the asyn
                                    * interfaces that can generate callbacks */
                ASYN_MULTIDEVICE, // Flags when creating the asyn port driver
                1,     // The autoConnect flag for the asyn port driver
                priority, // The thread priority for the asyn port driver thread
                stackSize // The stack size for the asyn port driver thread
            ),
      state_(NDAttrPlot_InitState),
      data_(),
      uids_(cache_size),
      n_attributes_(n_attributes),
      attributes_(),
      n_data_blocks_(n_data_blocks),
      data_selections_(n_data_blocks, ND_ATTRPLOT_NONE_INDEX),
      expose_task_(*this)
{
    data_.reserve(n_attributes_);
    for (size_t i = 0; i < n_attributes_; ++i) {
        data_.push_back(CB(cache_size));
    }

    createParam(NDAttrPlotDataString, asynParamFloat64Array, &NDAttrPlotData);
    createParam(NDAttrPlotDataLabelString, asynParamOctet,
            &NDAttrPlotDataLabel);
    createParam(NDAttrPlotDataSelectString, asynParamInt32,
            &NDAttrPlotDataSelect);
    createParam(NDAttrPlotAttributeString, asynParamOctet,
            &NDAttrPlotAttribute);
    createParam(NDAttrPlotResetString, asynParamInt32, &NDAttrPlotReset);
    createParam(NDAttrPlotNPtsString, asynParamInt32, &NDAttrPlotNPts);

    setStringParam(NDPluginDriverPluginType, "NDAttrPlot");

    callParamCallbacks();
}

void NDPluginAttrPlot::start_expose() {
    expose_task_.start();
}

void NDPluginAttrPlot::callback_data() {

    size_t size = uids_.size();
    size_t cache_size = uids_.max_size();
    double * const tmp_arr = new double[cache_size];

    size_t n_copied;
    for (size_t i = 0; i < n_data_blocks_; ++i) {
        int selected = data_selections_[i];
        if (selected == ND_ATTRPLOT_UID_INDEX) {
            n_copied = uids_.copy_to_array(tmp_arr, size);
        } else if (selected >= 0 &&
                static_cast<unsigned>(selected) < data_.size()) {
            n_copied = data_[selected].copy_to_array(tmp_arr,
                    size);
        } else {
            std::fill(tmp_arr, tmp_arr + size, epicsNAN);
            n_copied = size;
        }
        // To remove visual artifacts on EDM plots fill
        // the remaining arrays with the last point
        std::fill(tmp_arr + n_copied,
                tmp_arr + cache_size,
                *(tmp_arr + n_copied - 1));
        doCallbacksFloat64Array(tmp_arr, cache_size, NDAttrPlotData, (int)i);
    }

    delete[] tmp_arr;
}

void NDPluginAttrPlot::processCallbacks(NDArray *pArray) {
    NDAttributeList attr_list;

    NDPluginDriver::beginProcessCallbacks(pArray);
    pArray->pAttributeList->copy(&attr_list);

    epicsInt32 uid;
    getIntegerParam(NDUniqueId, &uid);
    if (uids_.size() != 0 && uid <= uids_.last()) {
        reset_data();
    }

    if (state_ == NDAttrPlot_InitState) {
        rebuild_attributes(attr_list);
        state_ = NDAttrPlot_ActiveState;
    }

    push_data(uid, attr_list);

    setIntegerParam(NDAttrPlotNPts, (int)uids_.size());
    callParamCallbacks();
}

void NDPluginAttrPlot::rebuild_attributes(NDAttributeList& attr_list) {
    std::vector<std::string> selections(n_data_blocks_);
    for (unsigned i = 0; i < n_data_blocks_; ++i) {
        int selection = data_selections_[i];
        if (selection == ND_ATTRPLOT_UID_INDEX) {
            selections[i] = ND_ATTRPLOT_UID_LABEL;
        } else if (selection >= 0 &&
                static_cast<unsigned>(selection) < attributes_.size()) {
            selections[i] = attributes_[selection];
        }
    }

    attributes_.clear();
    for (NDAttribute * attr = attr_list.next(NULL);
            attr != NULL && attributes_.size() <= n_attributes_;
            attr = attr_list.next(attr)) {
        std::string name(attr->getName());
        NDAttrDataType_t type = attr->getDataType();
        // This plugin only handles numeric types
        if (type == NDAttrInt8 ||
                type == NDAttrUInt8 ||
                type == NDAttrInt16 ||
                type == NDAttrUInt16 ||
                type == NDAttrInt32 ||
                type == NDAttrUInt32 ||
                type == NDAttrInt64 ||
                type == NDAttrUInt64 ||
                type == NDAttrFloat32 ||
                type == NDAttrFloat64)
        {
            attributes_.push_back(name);
        }
    }

    std::sort(attributes_.begin(), attributes_.end());

    for (unsigned i = 0; i < n_data_blocks_; ++i) {
        if (selections[i] == ND_ATTRPLOT_UID_LABEL) {
            data_selections_[i] = ND_ATTRPLOT_UID_INDEX;
        } else {
            std::vector<std::string>::const_iterator attr_it =
                std::find(attributes_.begin(), attributes_.end(),
                        selections[i]);
            if (attr_it != attributes_.end()) {
                data_selections_[i] = (int)(attr_it - attributes_.begin());
            } else {
                data_selections_[i] = ND_ATTRPLOT_NONE_INDEX;
            }
        }
    }

    callback_attributes();
    callback_selected();
}


void NDPluginAttrPlot::reset_data() {
    state_ = NDAttrPlot_InitState;
    uids_.clear();
    for (std::vector<CB>::iterator it = data_.begin();
            it != data_.end(); ++it) {
        it->clear();
    }
}

void NDPluginAttrPlot::callback_attributes() {
    for (int i = 0; i < (int)n_attributes_; i++) {
        if (i < (int)attributes_.size()) {
            setStringParam(i, NDAttrPlotAttribute, attributes_[i].c_str());
        } else {
            setStringParam(i, NDAttrPlotAttribute, "");
        }
        callParamCallbacks(i);
    }
}

void NDPluginAttrPlot::callback_selected() {
    for (int i = 0; i < (int)n_data_blocks_; i++) {
        int selected = data_selections_[i];
        std::string attr = ND_ATTRPLOT_NONE_LABEL;
        if (selected == ND_ATTRPLOT_UID_INDEX) {
            attr = ND_ATTRPLOT_UID_LABEL;
        } else if (selected >= 0 &&
                static_cast<size_t>(selected) < attributes_.size()) {
            attr = attributes_[selected];
        } else {
            selected = ND_ATTRPLOT_NONE_INDEX;
        }
        setStringParam(i, NDAttrPlotDataLabel, attr.c_str());
        setIntegerParam(i, NDAttrPlotDataSelect, selected);
        callParamCallbacks(i);
    }
}

asynStatus NDPluginAttrPlot::push_data(epicsInt32 uid, NDAttributeList& list) {
    size_t length = attributes_.size();
    double *new_values = new double[length];
    std::fill(new_values, new_values + length, epicsNAN);

    // Populate the new values with values from the attribute list
    for (NDAttribute * attr = list.next(NULL);
            attr != NULL; attr = list.next(attr)) {
        std::string name(attr->getName());
        std::vector<std::string>::const_iterator it = std::find(
                attributes_.begin(), attributes_.end(), name);
        if (it != attributes_.end()) {
            ptrdiff_t index = it - attributes_.begin();
            attr->getValue(NDAttrFloat64, &new_values[index], 1);
        }
    }

    // Push the new values to the data block
    uids_.push_back(uid);
    for (size_t i = 0; i < length; ++i) {
        data_[i].push_back(new_values[i]);
    }

    delete new_values;
    return asynSuccess;
}

asynStatus NDPluginAttrPlot::writeInt32(asynUser * pasynUser, epicsInt32 value)
{
    int reason = pasynUser->reason;
    int addr = -1;

    if (getAddress(pasynUser, &addr) != asynSuccess) {
        return asynError;
    }

    if (reason == NDAttrPlotDataSelect) {
        if (addr < 0 || static_cast<unsigned>(addr) >= n_data_blocks_) {
            return asynError;
        }
        if (value > 0 && static_cast<unsigned>(value) >= attributes_.size()) {
            return asynError;
        }
        data_selections_[addr] = value;
        callback_selected();
        callback_data();
        return asynSuccess;
    } else if (reason == NDAttrPlotReset) {
        reset_data();
        return asynSuccess;
    }

    return NDPluginDriver::writeInt32(pasynUser, value);
}

extern "C" {

NDPluginAttrPlot * plugin;

static void initHooks(initHookState state) {
    if (state == initHookAfterIocRunning) {
        plugin->start_expose();
    }
}

int NDAttrPlotConfig(const char * port, int n_attributes, int cache_size,
        int n_selected_blocks, const char * in_port, int in_addr,
        int queue_size, int blocking_callbacks, int priority, int stackSize) {
    plugin = new NDPluginAttrPlot(port, n_attributes, cache_size,
            n_selected_blocks, in_port, in_addr, queue_size, blocking_callbacks,
            priority, stackSize);
    initHookRegister(initHooks);
    plugin->start();

    return asynSuccess;
}

static const iocshArg portArg = {"Port name", iocshArgString};
static const iocshArg nAttributesArg = {"Number of attributes", iocshArgInt};
static const iocshArg cacheSizeArg = {"Cache size", iocshArgInt};
static const iocshArg nBlocksArg = {"Number of array blocks", iocshArgInt};
static const iocshArg inPortArg = {"Input Port name", iocshArgString};
static const iocshArg inAddrArg = {"Input address", iocshArgInt};
static const iocshArg queueSizeArg = {"Queue size", iocshArgInt};
static const iocshArg blockingCallbacksArg =
        {"Uses blocking callbacks", iocshArgInt};
static const iocshArg priorityArg = {"Thread priority", iocshArgInt};
static const iocshArg stackSizeArg = {"Thread stack size", iocshArgInt};

static const iocshArg * const NDAttrPlotArgs[] = {&portArg,
                                                 &nAttributesArg,
                                                 &cacheSizeArg,
                                                 &nBlocksArg,
                                                 &inPortArg,
                                                 &inAddrArg,
                                                 &queueSizeArg,
                                                 &blockingCallbacksArg,
                                                 &priorityArg,
                                                 &stackSizeArg};

static const iocshFuncDef NDAttrPlotCallConfig =
        {"NDAttrPlotConfig", 10, NDAttrPlotArgs};

static void NDAttrPlotCallFunc(const iocshArgBuf * args)
{
    NDAttrPlotConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival,
            args[4].sval, args[5].ival, args[6].ival, args[7].ival,
            args[8].ival, args[9].ival);
}

static void NDAttrPlotRegister() {
    iocshRegister(&NDAttrPlotCallConfig, NDAttrPlotCallFunc);
}

epicsExportRegistrar(NDAttrPlotRegister);

} //extern "C"

