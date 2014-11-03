/* simDetectorNoIOC.cpp
 *
 * This is an example of creating a simDetector and controlling it from outside an IOC
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  October 27, 2014
 *
 */

#include <asynPortClient.h>
#include <NDPluginStats.h>
#include <NDPluginStdArrays.h>
#include "simDetector.h"
#include <epicsThread.h>

void statsCounterCallback(void *drvPvt, asynUser *pasynUser, epicsInt32 data)
{
  if ((data % 10) == 0) {
    printf("statsCounterCallback, counter=%d\n", data);
  }
}

int main(int argc, char **argv)
{
  int numStd;
  int numStats;
  asynStatus status;

  simDetector                simD  = simDetector("SIM1", 1024, 1024, NDUInt8, 0, 0, 0, 0);
  NDPluginStats             stats  = NDPluginStats("STATS1", 20, 0, "SIM1", 0, 0, 0, 0, 0);
  NDPluginStdArrays     stdArrays  = NDPluginStdArrays("STD1", 20, 0, "SIM1", 0, 0, 0, 0);
  asynInt32Client         acquire  = asynInt32Client("SIM1",   0, ADAcquireString);
  asynInt32Client  arrayCallbacks  = asynInt32Client("SIM1",   0, NDArrayCallbacksString);
  asynInt32Client     enableStats  = asynInt32Client("STATS1", 0, NDPluginDriverEnableCallbacksString);
  asynInt32Client    statsCounter  = asynInt32Client("STATS1", 0, NDArrayCounterString);
  asynInt32Client       enableStd  = asynInt32Client("STD1",   0, NDPluginDriverEnableCallbacksString);
  asynInt32Client stdArraysCounter = asynInt32Client("STD1",   0, NDArrayCounterString);

  arrayCallbacks.write(1);
  enableStats.write(1);
  enableStd.write(1);
  status = statsCounter.registerInterruptUser(statsCounterCallback);
  if (status) {
    printf("Error calling registerInterruptUser, status=%d\n", status);
  }
  stats.report(stdout, 10);
  simD.report(stdout, 10);
  acquire.write(1);
  for (int i=0; i<10; i++) {
    stdArraysCounter.read(&numStd);
    printf("NDStdArrays callbacks=%d\n", numStd);
    statsCounter.read(&numStats);
    printf("NDStats callbacks    =%d\n", numStats);
    epicsThreadSleep(1.0);
  }

  return 0;
}


