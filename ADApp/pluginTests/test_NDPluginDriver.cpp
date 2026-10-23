/*
 * test_NDPluginDriver.cpp
 *
 * Tests NDPluginDriver input queue handling.
 *  Created on: 22 Oct 2026
 *   Author: Jakub Wlodek
 */

#include <vector>

#include "boost/test/unit_test.hpp"

#include <epicsEvent.h>
#include <epicsThread.h>
#include <asynNDArrayDriver.h>
#include <NDPluginDriver.h>

#include "testingutilities.h"
#include "AsynPortClientContainer.h"

#define QUEUE_SIZE 10

// Driver that can push NDArrays to its plugins on demand
class QueueTestDriver : public asynNDArrayDriver
{
public:
    QueueTestDriver(const std::string& port)
      : asynNDArrayDriver(port.c_str(), 1, 0, 0,
                          asynGenericPointerMask, asynGenericPointerMask, 0, 1, 0, 0)
    {
    }

    void sendArray(int uniqueId)
    {
        size_t dims[1] = {16};
        NDArray *pArray = pNDArrayPool->alloc(1, dims, NDUInt8, 0, NULL);
        pArray->uniqueId = uniqueId;
        doCallbacksGenericPointer(pArray, NDArrayData, 0);
        pArray->release();
    }
};

// Plugin that holds its processing thread inside processCallbacks so arrays back up in the queue
class QueueTestPlugin : public NDPluginDriver, public AsynPortClientContainer
{
public:
    QueueTestPlugin(const std::string& port, const std::string& detectorPort)
      : NDPluginDriver(port.c_str(), QUEUE_SIZE, 0, detectorPort.c_str(), 0, 1, 0, 0,
                       asynGenericPointerMask, asynGenericPointerMask, 0, 1, 0, 0, 1),
        AsynPortClientContainer(port),
        holdProcessing_(true),
        processingStarted_(epicsEventMustCreate(epicsEventEmpty)),
        releaseProcessing_(epicsEventMustCreate(epicsEventEmpty))
    {
    }

    ~QueueTestPlugin()
    {
        cleanup();
        epicsEventDestroy(processingStarted_);
        epicsEventDestroy(releaseProcessing_);
    }

    bool waitForProcessingStarted()
    {
        return epicsEventWaitWithTimeout(processingStarted_, 5.0) == epicsEventWaitOK;
    }

    void releaseProcessing()
    {
        lock();
        holdProcessing_ = false;
        unlock();
        epicsEventSignal(releaseProcessing_);
    }

    std::vector<int> getProcessedIds()
    {
        lock();
        std::vector<int> ids(processedIds_);
        unlock();
        return ids;
    }

protected:
    void processCallbacks(NDArray *pArray)
    {
        processedIds_.push_back(pArray->uniqueId);
        if (!holdProcessing_) return;
        unlock();
        epicsEventSignal(processingStarted_);
        epicsEventMustWait(releaseProcessing_);
        lock();
    }

private:
    bool holdProcessing_;
    epicsEventId processingStarted_;
    epicsEventId releaseProcessing_;
    std::vector<int> processedIds_;
};

struct QueueTestFixture
{
    QueueTestDriver *driver;
    QueueTestPlugin *plugin;

    QueueTestFixture()
    {
        std::string driverPort("simQueue"), pluginPort("testQueue");
        uniqueAsynPortName(driverPort);
        uniqueAsynPortName(pluginPort);

        driver = new QueueTestDriver(driverPort);
        plugin = new QueueTestPlugin(pluginPort, driverPort);
        plugin->start();
        plugin->write(NDPluginDriverEnableCallbacksString, 1);
        plugin->write(NDPluginDriverArrayPortString, driverPort);
    }

    ~QueueTestFixture()
    {
        plugin->releaseProcessing();
        delete plugin;
        delete driver;
    }

    // Arrays are counted as queued until processCallbacks returns
    bool waitForQueuedArrayCount(int expected)
    {
        for (int i = 0; i < 500; i++) {
            if (driver->getQueuedArrayCount() == expected) return true;
            epicsThreadSleep(0.01);
        }
        return false;
    }

    // Leaves array 0 held in processCallbacks and arrays 1..numQueued in the queue
    void fillQueue(int numQueued)
    {
        driver->sendArray(0);
        BOOST_REQUIRE(plugin->waitForProcessingStarted());
        for (int i = 1; i <= numQueued; i++) {
            driver->sendArray(i);
        }
        BOOST_REQUIRE_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE - numQueued);
        BOOST_REQUIRE_EQUAL(driver->getQueuedArrayCount(), numQueued + 1);
    }

    void checkQueueCleared(int expectedDropped)
    {
        BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverClearQueueString), 0);
        BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE);
        BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverDroppedArraysString), expectedDropped);
        // Only the array held in processCallbacks remains
        BOOST_CHECK_EQUAL(driver->getQueuedArrayCount(), 1);
    }

    void checkAllArraysReturnedToPool()
    {
        NDArrayPool *pool = driver->pNDArrayPool;
        BOOST_CHECK_EQUAL(pool->getNumFree(), pool->getNumBuffers());
    }
};

BOOST_FIXTURE_TEST_SUITE(NDPluginDriverQueueTests, QueueTestFixture)

BOOST_AUTO_TEST_CASE(test_clear_queue_while_receiving_arrays)
{
    fillQueue(5);

    plugin->write(NDPluginDriverClearQueueString, 1);
    checkQueueCleared(5);

    // Plugin still accepts new arrays after the queue is cleared
    driver->sendArray(6);
    driver->sendArray(7);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE - 2);

    plugin->releaseProcessing();
    BOOST_REQUIRE(waitForQueuedArrayCount(0));

    std::vector<int> expected;
    expected.push_back(0);
    expected.push_back(6);
    expected.push_back(7);
    std::vector<int> processed = plugin->getProcessedIds();
    BOOST_CHECK_EQUAL_COLLECTIONS(processed.begin(), processed.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverDroppedArraysString), 5);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE);
    checkAllArraysReturnedToPool();
}

BOOST_AUTO_TEST_CASE(test_clear_queue_with_callbacks_disabled)
{
    fillQueue(5);

    plugin->write(NDPluginDriverEnableCallbacksString, 0);
    plugin->write(NDPluginDriverClearQueueString, 1);
    checkQueueCleared(5);

    // These never reach the plugin
    driver->sendArray(6);
    driver->sendArray(7);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE);

    plugin->releaseProcessing();
    BOOST_REQUIRE(waitForQueuedArrayCount(0));

    std::vector<int> processed = plugin->getProcessedIds();
    BOOST_REQUIRE_EQUAL(processed.size(), (size_t)1);
    BOOST_CHECK_EQUAL(processed[0], 0);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverDroppedArraysString), 5);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE);
    checkAllArraysReturnedToPool();
}

BOOST_AUTO_TEST_CASE(test_clear_empty_queue)
{
    plugin->write(NDPluginDriverClearQueueString, 1);

    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverClearQueueString), 0);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverQueueFreeString), QUEUE_SIZE);
    BOOST_CHECK_EQUAL(plugin->readInt(NDPluginDriverDroppedArraysString), 0);
}

BOOST_AUTO_TEST_SUITE_END()
