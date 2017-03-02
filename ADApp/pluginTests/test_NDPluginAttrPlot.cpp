/*
 * test_NDPluginAttrPlot.cpp
 *
 *  Created on: 28 Feb 2016
 *      Author: Blaz Kranjc
 */

#include <stdio.h>

#include "NDPluginAttrPlot.h"
#include "AttrPlotPluginWrapper.h"

#include "boost/test/unit_test.hpp"

// AD dependencies
#include <NDPluginDriver.h>
#include <NDArray.h>
#include <NDAttribute.h>
#include <asynDriver.h>

#include <string.h>
#include <stdint.h>

#include <algorithm>
#include <deque>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "testingutilities.h"
#include "AsynException.h"


class NDArrayWrapper {
    public:
        ~NDArrayWrapper() {
            delete array_;
        }
        NDArray* get() {
            return array_;
        }
        NDArrayWrapper()
            : array_(new NDArray)
        { }
        NDArrayWrapper& set_uid(int uid) {
            array_->uniqueId = uid;
            return *this;
        }
        NDArrayWrapper& add_attr(const std::string& name,
               int value) {
            array_->pAttributeList->add(name.c_str(), "",
                   NDAttrInt32, &value);
            return *this;
        }
        NDArrayWrapper& add_attr(const std::string& name,
               double value) {
            array_->pAttributeList->add(name.c_str(), "",
                   NDAttrFloat64, &value);
            return *this;
        }
        NDArrayWrapper& add_attr(const std::string& name,
               unsigned value) {
            array_->pAttributeList->add(name.c_str(), "",
                   NDAttrUInt32, &value);
            return *this;
        }
        NDArrayWrapper& add_attr(const std::string& name,
               const std::string& value) {
            array_->pAttributeList->add(name.c_str(), "",
                   NDAttrString, const_cast<char*>(&(value.c_str())[0]));
            return *this;
        }
    private:
        NDArray* array_;
};

struct AttrPlotPluginTestFixture
{
    static const int cache_size = 10;
    static const int n_attributes = 3;
    static const int n_selected = 2;
    AttrPlotPluginWrapper* attrPlot;

    AttrPlotPluginTestFixture()
    {

        // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers
        // (even if only one is ever instantiated at once), so we change it slightly for each test case.
        std::string testport("TS");
        uniqueAsynPortName(testport);

        // This is the plugin under test
        attrPlot = new AttrPlotPluginWrapper(
                    testport, n_attributes, cache_size, n_selected,
                    "", 0);

        // Enable the plugin
        attrPlot->start(); // start the plugin thread
        attrPlot->write(NDPluginDriverEnableCallbacksString, 1);
        attrPlot->write(NDPluginDriverBlockingCallbacksString, 1);
    }

    ~AttrPlotPluginTestFixture()
    {
        delete attrPlot;
    }

};

BOOST_FIXTURE_TEST_SUITE(AttrPlotPluginTests, AttrPlotPluginTestFixture)

BOOST_AUTO_TEST_CASE(attrplot_attribute_data)
{
    const std::string attr_name = "attribute";

    // Enable plugin
    BOOST_CHECK_NO_THROW(attrPlot->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDArrayCallbacksString), 1);

    // Values to compare the cache with
    std::vector<double> values;
    // Fill the cache with arrays with a single attribute
    for (int i = 0; i < cache_size * 2; ++i) {
        double value = i*i;
        NDArrayWrapper wrap;
        wrap.set_uid(i).add_attr(attr_name, value);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();

        // Select the attribute
        BOOST_CHECK_NO_THROW(attrPlot->write(NDAttrPlotDataSelectString, 0, 0));
        BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 0),
                0);

        values.push_back(value);
        int array_count = i + 1; // i starts with 0, when number of arrays is 1
        int nPts = array_count < cache_size ? array_count : cache_size;
        BOOST_CHECK_EQUAL(attrPlot->readInt("ARRAY_COUNTER"), array_count);
        BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString),
                nPts);
        // Check that the data is OK
        // TODO Need to extend the functionality of the AsynPortClientContainer
    }
}

BOOST_AUTO_TEST_CASE(attrplot_attribute_select)
{
    const std::string attr_name = "attribute";

    // Enable plugin
    BOOST_CHECK_NO_THROW(attrPlot->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDArrayCallbacksString), 1);

    // Fill the cache with array with a single attribute
    NDArrayWrapper wrap;
    wrap.set_uid(1).add_attr(attr_name, 0);

    attrPlot->lock();
    BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
    attrPlot->unlock();

    // Check that only one of the attribute strings is set
    for (int i = 0; i < n_attributes; ++i) {
        BOOST_CHECK_EQUAL(attrPlot->readString(NDAttrPlotAttributeString, i),
                i == 0 ? attr_name : "");
    }

    // Ensure nothing is selected before selecting the data
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 0),
            ND_ATTRPLOT_NONE_INDEX);
    // We must be able to select UID
    BOOST_CHECK_NO_THROW(attrPlot->write(NDAttrPlotDataSelectString,
                ND_ATTRPLOT_UID_INDEX, 0));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 0),
            ND_ATTRPLOT_UID_INDEX);
    BOOST_CHECK_EQUAL(attrPlot->readString(NDAttrPlotDataLabelString, 0),
            ND_ATTRPLOT_UID_LABEL);
    // The first (0) attribute is allowed to be selectable
    BOOST_CHECK_NO_THROW(attrPlot->write(NDAttrPlotDataSelectString, 0, 0));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 0),
            0);
    BOOST_CHECK_EQUAL(attrPlot->readString(NDAttrPlotDataLabelString, 0),
            attr_name);
    // The other attributes are not
    for (int i = 1; i < n_attributes; ++i) {
        BOOST_CHECK_THROW(attrPlot->write(NDAttrPlotDataSelectString, i, 0),
                AsynException);
    }

}

BOOST_AUTO_TEST_CASE(attrplot_string_attribute)
{
    // Enable plugin
    BOOST_CHECK_NO_THROW(attrPlot->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDArrayCallbacksString), 1);

    NDArrayWrapper wrap;
    wrap.set_uid(1).add_attr("string", "string");

    attrPlot->lock();
    // Ensure that the plugin works even if string attributes are included
    BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
    attrPlot->unlock();

    // Check that none of the attribute strings are set as string attrs are not
    // supported
    for (int i = 0; i < n_attributes; ++i) {
        BOOST_CHECK_EQUAL(attrPlot->readString(NDAttrPlotAttributeString, i),
                "");
    }
}

BOOST_AUTO_TEST_CASE(attrplot_reset_uid)
{
    // Enable plugin
    BOOST_CHECK_NO_THROW(attrPlot->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDArrayCallbacksString), 1);

    for (int i = 0; i < cache_size/2; ++i) {
        NDArrayWrapper wrap;
        wrap.set_uid(i);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();

        int array_count = i + 1; // i starts with 0, when number of arrays is 1
        int nPts = array_count < cache_size ? array_count : cache_size;
        BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString),
                nPts);
    }

    { // Send another array but with a uid 0 which must reset the plugin
        NDArrayWrapper wrap;
        wrap.set_uid(0);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();

        BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString), 1);
    }
}

BOOST_AUTO_TEST_CASE(attrplot_reset_pv_write)
{
    // Enable plugin
    BOOST_CHECK_NO_THROW(attrPlot->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDArrayCallbacksString), 1);

    int n_arrays = cache_size/2;
    for (int i = 0; i < n_arrays; ++i) {
        NDArrayWrapper wrap;
        wrap.set_uid(i);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();

        int array_count = i + 1; // i starts with 0, when number of arrays is 1
        int nPts = array_count < cache_size ? array_count : cache_size;
        BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString),
                nPts);
    }

    { // Send command to reset the plugin which must reset the plugin on next write
        BOOST_CHECK_NO_THROW(attrPlot->write(NDAttrPlotResetString, 1));

        NDArrayWrapper wrap;
        wrap.set_uid(n_arrays);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();

        BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString), 1);
    }
}

BOOST_AUTO_TEST_CASE(attrplot_multiple_attributes)
{
    // Enable plugin
    BOOST_CHECK_NO_THROW(attrPlot->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDArrayCallbacksString), 1);

    // initialize the driver with NDArray with three attributes
    std::vector<std::string> attributes;
    attributes.push_back("attribute1");
    attributes.push_back("attribute2");
    attributes.push_back("attribute3");
    {
        NDArrayWrapper wrap;
        wrap.set_uid(0)
            .add_attr(attributes[0], 0)
            .add_attr(attributes[1], 0)
            .add_attr(attributes[2], 0);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();
    }

    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString), 1);
    // Check that all attributes are read by the driver
    std::string attr1 = attrPlot->readString(NDAttrPlotAttributeString, 0);
    std::string attr2 = attrPlot->readString(NDAttrPlotAttributeString, 1);
    std::string attr3 = attrPlot->readString(NDAttrPlotAttributeString, 2);
    BOOST_CHECK(attr1 != attr2 and attr2 != attr3 and attr1 != attr3);
    BOOST_CHECK(std::find(attributes.begin(), attributes.end(), attr1) != attributes.end());
    BOOST_CHECK(std::find(attributes.begin(), attributes.end(), attr2) != attributes.end());
    BOOST_CHECK(std::find(attributes.begin(), attributes.end(), attr3) != attributes.end());

    // We select the attr2 and attr3 to be exposed
    BOOST_CHECK_NO_THROW(attrPlot->write(NDAttrPlotDataSelectString, 1, 0));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 0), 1);
    BOOST_CHECK_NO_THROW(attrPlot->write(NDAttrPlotDataSelectString, 2, 1));
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 1), 2);

    { // Send a new array with just attr2
        NDArrayWrapper wrap;
        wrap.set_uid(0) // We want reset to occur
            .add_attr(attr2, 0);

        attrPlot->lock();
        BOOST_CHECK_NO_THROW(attrPlot->processCallbacks(wrap.get()));
        attrPlot->unlock();
    }

    // Check that only attr2 is read
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotNPtsString), 1);
    for (int i = 0; i < n_attributes; ++i) {
        BOOST_CHECK_EQUAL(attrPlot->readString(NDAttrPlotAttributeString, i),
                i == 0 ? attr2 : "");
    }

    // Check that attr2 is still selected and that the attr3's slot is no longer selected
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 0),
            0);
    BOOST_CHECK_EQUAL(attrPlot->readString(NDAttrPlotDataLabelString, 0),
            attr2);
    BOOST_CHECK_EQUAL(attrPlot->readInt(NDAttrPlotDataSelectString, 1),
            ND_ATTRPLOT_NONE_INDEX);
}

BOOST_AUTO_TEST_SUITE_END()
