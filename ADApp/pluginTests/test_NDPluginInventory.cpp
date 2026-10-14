/*
 * test_NDPluginInventory.cpp
 *
 *  Created on: 15 Sep 2026
 *      Author: Jakub Wlodek
 */
#include <string>
#include <vector>

#include "boost/test/unit_test.hpp"

#include <NDPluginInventory.h>
#include <asynNDArrayDriver.h>
#include <asynDriver.h>

#include "testingutilities.h"
#include "CodecPluginWrapper.h"
#include "HDF5PluginWrapper.h"
#include "ROIPluginWrapper.h"

using namespace std;

struct PluginInventoryTestFixture
{
    asynNDArrayDriver *dummy_driver;
    vector<CodecPluginWrapper*> codecs;
    HDF5PluginWrapper *hdf;
    vector<ROIPluginWrapper*> rois;
    vector<string> codecPorts;
    string hdfPort;
    vector<string> roiPorts;

    /** Create a dummy port driver with two codec, one HDF5 and, four ROI plugins. */
    PluginInventoryTestFixture()
    {
        string dummy_port("simInvPort");
        uniqueAsynPortName(dummy_port);

        dummy_driver = new asynNDArrayDriver(dummy_port.c_str(), 1, 0, 0,
                asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 0);

        // Two codec plugins.
        for (int i = 0; i < 2; i++) {
            string port("invCodec");
            uniqueAsynPortName(port);
            codecPorts.push_back(port);
            CodecPluginWrapper *codec = new CodecPluginWrapper(port, dummy_port);
            // Plugin type is set in start()
            codec->start();
            // PV prefixes are set in writeOctet() when the PvPrefix record PINI is processed.
            codec->write(NDPluginDriverPvPrefixString, "TEST:CODEC" + to_string(i + 1) + ":");
            codecs.push_back(codec);
        }

        // One HDF5 plugin.
        hdfPort = "invHDF";
        uniqueAsynPortName(hdfPort);
        hdf = new HDF5PluginWrapper(hdfPort, dummy_port);
        hdf->start();
        hdf->write(NDPluginDriverPvPrefixString, "TEST:HDF:");

        // Four ROI plugins.
        for (int i = 0; i < 4; i++) {
            string port("invROI");
            uniqueAsynPortName(port);
            roiPorts.push_back(port);
            ROIPluginWrapper *roi = new ROIPluginWrapper(port, dummy_port);
            roi->start();
            roi->write(NDPluginDriverPvPrefixString, "TEST:ROI" + to_string(i + 1) + ":");
            rois.push_back(roi);
        }
    }

    ~PluginInventoryTestFixture()
    {
        for (size_t i = 0; i < codecs.size(); i++) delete codecs[i];
        delete hdf;
        for (size_t i = 0; i < rois.size(); i++) delete rois[i];
        delete dummy_driver;
    }

    // Finds the plugin for a given port within a type's plugin list.
    const NDPluginInventory::RegisteredPlugin *findPlugin(
            const vector<NDPluginInventory::RegisteredPlugin> &plugins,
            const string &portName)
    {
        for (size_t i = 0; i < plugins.size(); i++) {
            if (plugins[i].portName == portName) return &plugins[i];
        }
        return NULL;
    }
};

BOOST_FIXTURE_TEST_SUITE(PluginInventoryTests, PluginInventoryTestFixture)

// Plugins group under their plugin type, carrying port name and PV prefix.
BOOST_AUTO_TEST_CASE(test_InventoryKeyedByType)
{
    NDPluginInventory::Inventory inv = NDPluginInventory::getInventory();

    NDPluginInventory::Inventory::const_iterator codecIt = inv.find("NDPluginCodec");
    BOOST_REQUIRE(codecIt != inv.end());
    BOOST_CHECK_EQUAL(codecIt->second.size(), 2u);
    for (size_t i = 0; i < codecPorts.size(); i++) {
        const NDPluginInventory::RegisteredPlugin *e = findPlugin(codecIt->second, codecPorts[i]);
        BOOST_REQUIRE(e != NULL);
        BOOST_CHECK_EQUAL(e->pvPrefix, "TEST:CODEC" + to_string(i + 1) + ":");
    }

    NDPluginInventory::Inventory::const_iterator hdfIt = inv.find("NDFileHDF5");
    BOOST_REQUIRE(hdfIt != inv.end());
    BOOST_CHECK_EQUAL(hdfIt->second.size(), 1u);
    const NDPluginInventory::RegisteredPlugin *hdfEntry = findPlugin(hdfIt->second, hdfPort);
    BOOST_REQUIRE(hdfEntry != NULL);
    BOOST_CHECK_EQUAL(hdfEntry->pvPrefix, "TEST:HDF:");

    NDPluginInventory::Inventory::const_iterator roiIt = inv.find("NDPluginROI");
    BOOST_REQUIRE(roiIt != inv.end());
    BOOST_CHECK_EQUAL(roiIt->second.size(), 4u);
    for (size_t i = 0; i < roiPorts.size(); i++) {
        const NDPluginInventory::RegisteredPlugin *e = findPlugin(roiIt->second, roiPorts[i]);
        BOOST_REQUIRE(e != NULL);
        BOOST_CHECK_EQUAL(e->pvPrefix, "TEST:ROI" + to_string(i + 1) + ":");
    }
}

// A plugin is removed from the inventory when it is destroyed.
BOOST_AUTO_TEST_CASE(test_UnregisterOnDelete)
{
    delete rois[3];
    rois[3] = NULL;

    NDPluginInventory::Inventory inv = NDPluginInventory::getInventory();
    NDPluginInventory::Inventory::const_iterator it = inv.find("NDPluginROI");
    BOOST_REQUIRE(it != inv.end());

    BOOST_CHECK_EQUAL(it->second.size(), 3u);
    BOOST_CHECK(findPlugin(it->second, roiPorts[3]) == NULL);
    BOOST_CHECK(findPlugin(it->second, roiPorts[0]) != NULL);
}

BOOST_AUTO_TEST_SUITE_END()
