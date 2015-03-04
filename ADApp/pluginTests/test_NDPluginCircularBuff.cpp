/* Nothing here yet*/ 
#include <stdio.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "NDPluginCircularBuff Tests"
#include "boost/test/unit_test.hpp"

// AD dependencies
#include "NDPluginCircularBuff.h"
#include "simDetector.h"
#include "NDPluginROI.h"
#include "NDArray.h"
#include "asynDriver.h"

BOOST_AUTO_TEST_SUITE(CircularBuffTests)

BOOST_AUTO_TEST_CASE(test1)
{
    NDArrayPool *arrayPool = new NDArrayPool(100, 0);
  // simDetectorConfig(portName, maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory)
  simDetector *driver = new simDetector("simPort", 800, 500, NDFloat64, 50, 0, 0, 2000000);
  NDPluginCircularBuff *cb = new NDPluginCircularBuff("testPort", 50, 0, "simPort", 0, 1000, -1, 0, 2000000);
  NDPluginROI *roi = new NDPluginROI("roiPort", 16, 1, "testPort", 0, 50, -1, 0, 2000000);

  // pasynManager is a global declared in asynDriver.h and defined in asynManager.c
  // We need to supply a valid-looking asynUser to calls to writeInt32 and its ilk, since they make
  // calls to epicsSnprintf which apparently expects some of the member of the asynUser to actually
  // point to something - we get a segfault if we simply initialise an asynUser struct and fill in
  // the reason.
  asynUser *user = pasynManager->createAsynUser(0,0);
  // readOctet segfaults if we don't supply something for its eom and actual bytes arguments
  size_t gotbytes;
  int eom;

  // To store values read back from driver
  int storedImages;
  char status[50] = {0};

  // Enable callbacks on the downstream plugin, so we can check how many arrays it received from the circular buff.
  user->reason = 53 - 3; // NDPluginDriverEnableCallbacks
  roi->lock();
  roi->writeInt32(user, 1);
  roi->unlock();
  user->reason = 53 - 2; //NDPluginDriverBlockingCallbacks
  roi->lock();
  roi->writeInt32(user, 1);
  roi->unlock();

  // It would be nice to be able to access these directly, but FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM
  // is a private member variable of the plugin.
  user->reason = 53 + 1; // NDPluginCircularBuffStatus
  cb->readOctet(user, status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Idle"), 0);

  user->reason = 53 + 2; // NDPluginCircularBuffPreTrigger
  cb->writeInt32(user, 10);

  user->reason = 53 + 0; // NDPluginCircularBuffControl
  cb->writeInt32(user, 1);

  printf("Got this far\n");

  user->reason = 53 + 1; // NDPluginCircularBuffStatus
  cb->readOctet(user, status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer filling"), 0);

  size_t dims[2] = {2,5};
  NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

  cb->processCallbacks(testArray);
  BOOST_REQUIRE_NE(cb, (NDPluginCircularBuff *)NULL);

  user->reason = 53 + 4; // NDPluginCircularBuffCurrentImage

  cb->readInt32(user, &storedImages);
  user->reason = 53 + 1; // NDPluginCircularBuffStatus

  cb->readOctet(user, status, 50, &gotbytes, &eom);

  BOOST_REQUIRE_EQUAL(storedImages, 1);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer filling"), 0);

  for (int i = 0; i < 10; i++)
      cb->processCallbacks(testArray);


  user->reason = 53 + 4; // NDPluginCircularBuffCurrentImage

  cb->readInt32(user, &storedImages);
  user->reason = 53 + 1; // NDPluginCircularBuffStatus

  cb->readOctet(user, status, 50, &gotbytes, &eom);

  printf("%s\n", status);
  BOOST_REQUIRE_EQUAL(storedImages, 10);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer Wrapping"), 0);

  printf("Status : %s\n", status);

  // Trigger the buffer flush
  user->reason = 53 + 6; // NDPluginCircularBuffSoftTrigger
  cb->writeInt32(user, 1);

  cb->lock();
  cb->processCallbacks(testArray);
  cb->unlock();

  user->reason = 53 + 4; // NDPluginCircularBuffCurrentImage
  cb->readInt32(user, &storedImages);

  //BOOST_REQUIRE_EQUAL(storedImages, 0);

  user->reason = 53 - 9 - 30; //NDArrayCounter, hopefully
  int arrayCount;
  cb->readInt32(user, &arrayCount);

  BOOST_REQUIRE_EQUAL(arrayCount, 12);

  // Should have received the 10 buffered frames plus the trigger frame.
  roi->readInt32(user, &arrayCount);
  BOOST_REQUIRE_EQUAL(arrayCount, 11);

  delete cb;
  delete driver;
}

BOOST_AUTO_TEST_CASE(case2)
{
  int a = 1;
  int b = 1;
  BOOST_REQUIRE_EQUAL(a,b);
}

BOOST_AUTO_TEST_SUITE_END()
