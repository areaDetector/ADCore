#ifndef NDATTRPLOT_H_
#define NDATTRPLOT_H_

/**
 * NDAttrPlot.cpp
 *
 * Plugin that caches NDAttribute values for an acquisition and exposes values
 * of the selected ones to the EPICS layer periodically.
 *
 * Relevant templates:
 *   - NDAttrPlot.template     Base template for the plugin
 *   - NDAttrPlotData.template Records for data exposure, there should be
 *                             max_selected instances loaded
 *   - NDAttrPlotAttr.template Records for attribute names, there should be
 *                             max_attributes instances loaded
 */

#include "CircularBuffer.h"

#include <NDPluginAPI.h>
#include <NDPluginDriver.h>
#include <epicsThread.h>

#include <string>
#include <vector>

#define NDAttrPlotDataString        "AP_Data"
#define NDAttrPlotDataLabelString   "AP_DataLabel"
#define NDAttrPlotDataSelectString  "AP_DataSelect"
#define NDAttrPlotAttributeString   "AP_Attribute"
#define NDAttrPlotResetString       "AP_Reset"
#define NDAttrPlotNPtsString        "AP_NPts"

#define ND_ATTRPLOT_UID_INDEX        -1
#define ND_ATTRPLOT_UID_LABEL        "UID"
#define ND_ATTRPLOT_NONE_INDEX       -2
#define ND_ATTRPLOT_NONE_LABEL       "None"

#define ND_ATTRPLOT_DATA_EXPOSURE_PERIOD 1. // Data callback period in seconds

class NDPluginAttrPlot;

/**
 * A task that periodically executes the data exposure method of the plugin.
 */
class NDPLUGIN_API ExposeDataTask : public epicsThreadRunable
{
public:
    /**
     * Constructor.
     *
     * \param plugin Reference to the plugin.
     */
    ExposeDataTask(NDPluginAttrPlot& plugin);

    /**
     * Periodically runs the data exposure on the plugin.
     */
    void run();

    /**
     * Start the internal thread of the task.
     */
    void start();

    /**
     * Stop the internal thread of the task.
     */
    void stop();

private:
    /** Plugin that the task acts on */
    NDPluginAttrPlot& plugin_;

    /** Stop flag that is used to stop the task */
    bool stop_;

    /** Thread running the task */
    epicsThread thread_;
};

/**
 * \brief AD plugin that saves attribute values from recieved NDArrays.
 *
 * The plugin caches last attributes values in and exposes
 * the selected ones periodically to the EPICS layer in waveform records. The
 * plugin only works with numerical Attributes. The type of the attribute is
 * discarded, all attributes are interpreted as doubles.
 *
 * The attributes are read from the NDArray on the first frame of acquisition
 * and populated in first come first served fashion (unpredictable order).
 * On reset or reacquistion the cache is cleared and all data is discarded.
 */
class NDPLUGIN_API NDPluginAttrPlot : public NDPluginDriver
{
    typedef CircularBuffer<double> CB;

public:
    /**
     * \brief Constructor.
     *
     * \param port Port name of the plugin.
     * \param max_attributes Maximum number of Y1 attributes to save.
     * \param cache_size Size of the cache for an attribute.
     * \param max_selected Maximum number of selected Y1 attributes.
     * \param in_port Port from where the NDArrays are recieved.
     * \param in_addr Address from where the NDArrays are recieved.
     * \param queue_size Size of the NDArray callback queue.
     * \param blocking_callbacks Should blocking callbacks be used.
     * \param priority Priority of the internal thread.
     * \param stackSize Stack size for the internal thread.
     */
    NDPluginAttrPlot(const char * port, int max_attributes, int cache_size,
            int max_selected, const char * in_port, int in_addr, int queue_size,
            int blocking_callbacks, int priority, int stackSize);

    /**
     * \brief Starts the data exposure task.
     */
    void start_expose();

    friend class ExposeDataTask;

public:
    /**
     * \brief Caches the values of attributes in the array.
     *
     * Also resets the driver if the UID of the NDArray is lower
     * than the last cached one.
     *
     * \param pArray The NDArray from the callback.
     */
    void processCallbacks(NDArray * pArray);

    /**
     * \brief Called when an OUT record with asynInt32 interface is processed.
     *
     * \param pasynUser Structure that encodes the reason and address.
     * \param value Value to write.
     */
    asynStatus writeInt32(asynUser * pasynUser, epicsInt32 value);

protected:
    int NDAttrPlotData;
#define NDATTRPLOT_FIRST_PARAM NDAttrPlotData
    int NDAttrPlotDataLabel;
    int NDAttrPlotDataSelect;
    int NDAttrPlotAttribute;
    int NDAttrPlotReset;
    int NDAttrPlotNPts;

private:
    /**
     * State of the plugin.
     */
    enum NDAttrPlotState {
        NDAttrPlot_InitState,
        NDAttrPlot_ActiveState
    };

    /**
     * \brief Repopulates the attributes from the attribute list.
     *
     * The new attribute names vector is ordered and contains at
     * most max_attributes attribute names.
     *
     * This also updates the selected fields so that if attributes
     * were selected before the rebuild they are also selected after
     * the rebuild.
     *
     * \param attr_list Attribute list from where the attributes are read.
     */
    void rebuild_attributes(NDAttributeList& attr_list);

    /**
     * \brief Clears all the data from the cache and reinitializes the plugin.
     */
    void reset_data();

    /**
     * \brief Sets data from the attribute list to the cache.
     * \param uid UniqueId of this NDArray
     * \param attr_list Attribute list containing the data.
     */
    asynStatus push_data(epicsInt32 uid, NDAttributeList& attr_list);

    /**
     * \brief Exposes the selected data fields to EPICS layer.
     */
    void callback_data();

    /**
     * \brief Exposes the attribute names to the EPICS layer.
     */
    void callback_attributes();

    /**
     * \brief Exposes the selection status of attributes to the EPICS layer.
     */
    void callback_selected();

private:
    /** Internal state of the plugin */
    NDAttrPlotState state_;

    /** Data arrays for each of the attributes */
    std::vector<CB> data_;

    /** Container for uids */
    CB uids_;

    /** Maximum number of saved attributes */
    const size_t n_attributes_;

    /** Attribute names of the saved data */
    std::vector<std::string> attributes_;

    const unsigned n_data_blocks_;
    std::vector<int> data_selections_;

    /** Task that periodically exposes the data */
    ExposeDataTask expose_task_;
};

#endif // NDATTRPLOT_H_

