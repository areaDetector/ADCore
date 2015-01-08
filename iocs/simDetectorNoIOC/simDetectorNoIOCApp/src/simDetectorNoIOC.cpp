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

#include <epicsThread.h>
#include <asynPortClient.h>
#include <NDPluginStats.h>
#include <NDPluginStdArrays.h>
#include <NDFileHDF5.h>
#include <simDetector.h>

#define NUM_FRAMES 10
#define FILE_PATH "/home/epics/scratch/";
#define REPORT_LEVEL 10

#ifndef EPICS_LIBCOM_ONLY
  #include <dbAccess.h>
#endif

void statsCounterCallback(void *drvPvt, asynUser *pasynUser, epicsInt32 data)
{
    printf("statsCounterCallback, counter=%d\n", data);
}

void acquireFrame(asynInt32Client *acquire)
{
  int acquiring;
  acquire->write(1);
  do {
    acquire->read(&acquiring);
    epicsThreadSleep(0.1);
  } while (acquiring);
}

int main(int argc, char **argv)
{
  int numStd;
  int numStats;
  int numHDF5Captured;
  double meanCounts;
  size_t numWritten;
  asynStatus status;
  const char *str;
  
#ifndef EPICS_LIBCOM_ONLY
  // Must set this for callbacks to work if EPICS_LIBCOM_ONLY is not defined
  interruptAccept = 1;  
#endif

  simDetector                   simD  =       simDetector("SIM1", 1024, 1024, NDUInt8, 0, 0, 0, 0);
  NDPluginStats                stats  =     NDPluginStats("STATS1", 20, 0, "SIM1", 0, 0, 0, 0, 0);
  NDPluginStdArrays        stdArrays  = NDPluginStdArrays("STD1",   20, 0, "SIM1", 0, 0, 0, 0);
  NDFileHDF5                fileHDF5  =        NDFileHDF5("HDF5",   20, 0, "SIM1", 0, 0, 0);
  asynInt32Client            acquire  =   asynInt32Client("SIM1",   0, ADAcquireString);
  asynInt32Client        acquireMode  =   asynInt32Client("SIM1",   0, ADImageModeString);
  asynInt32Client     arrayCallbacks  =   asynInt32Client("SIM1",   0, NDArrayCallbacksString);
  asynFloat64Client          simGain  = asynFloat64Client("SIM1",   0, ADGainString);
  asynInt32Client        statsEnable  =   asynInt32Client("STATS1", 0, NDPluginDriverEnableCallbacksString);
  asynInt32Client       statsCounter  =   asynInt32Client("STATS1", 0, NDArrayCounterString);
  asynInt32Client       statsCompute  =   asynInt32Client("STATS1", 0, NDPluginStatsComputeStatisticsString);
  asynFloat64Client        statsMean  = asynFloat64Client("STATS1", 0, NDPluginStatsMeanValueString);
  asynInt32Client    stdArraysEnable  =   asynInt32Client("STD1",   0, NDPluginDriverEnableCallbacksString);
  asynInt32Client   stdArraysCounter  =   asynInt32Client("STD1",   0, NDArrayCounterString);
  asynInt32Client         hdf5Enable  =   asynInt32Client("HDF5",   0, NDPluginDriverEnableCallbacksString);
  asynOctetClient           hdf5Name  =   asynOctetClient("HDF5",   0, NDFileNameString);
  asynOctetClient           hdf5Path  =   asynOctetClient("HDF5",   0, NDFilePathString);
  asynOctetClient       hdf5Template  =   asynOctetClient("HDF5",   0, NDFileTemplateString);
  asynInt32Client         hdf5Number  =   asynInt32Client("HDF5",   0, NDFileNumberString);
  asynInt32Client     hdf5NumCapture  =   asynInt32Client("HDF5",   0, NDFileNumCaptureString);
  asynInt32Client    hdf5NumCaptured  =   asynInt32Client("HDF5",   0, NDFileNumCapturedString);
  asynInt32Client      hdf5WriteMode  =   asynInt32Client("HDF5",   0, NDFileWriteModeString);
  asynInt32Client        hdf5Capture  =   asynInt32Client("HDF5",   0, NDFileCaptureString);

  arrayCallbacks.write(1);
  simGain.write(1.0);
  acquireMode.write(ADImageSingle);
  statsEnable.write(1);
  statsCompute.write(1);
  stdArraysEnable.write(1);
  hdf5Enable.write(1);
  str = "test";
  hdf5Name.write(str, strlen(str), &numWritten);
  str = FILE_PATH;
  hdf5Path.write(str, strlen(str), &numWritten);
  str = "%s%s_%3.3d.h5";
  hdf5Template.write(str, strlen(str), &numWritten);
  hdf5Number.write(1);
  hdf5NumCapture.write(NUM_FRAMES);
  hdf5WriteMode.write(NDFileModeStream);
  status = statsCounter.registerInterruptUser(statsCounterCallback);
  if (status) {
    printf("Error calling registerInterruptUser, status=%d\n", status);
  }
  pasynManager->report(stdout, REPORT_LEVEL, "SIM1");
  pasynManager->report(stdout, REPORT_LEVEL, "STATS1");
  // Acquire 1 frame so HDF5 plugin is configured
  acquireFrame(&acquire);
  hdf5Capture.write(1);
  pasynManager->report(stdout, REPORT_LEVEL, "HDF5");
  for (int i=0; i<NUM_FRAMES; i++) {
    acquireFrame(&acquire);
    stdArraysCounter.read(&numStd);
    printf("\nNDStdArrays callbacks=%d\n", numStd);
    statsCounter.read(&numStats);
    printf("NDStats callbacks    =%d\n", numStats);
    statsMean.read(&meanCounts);
    printf("NDStats mean counts =%f\n", meanCounts);
    hdf5NumCaptured.read(&numHDF5Captured);
    printf("HDF5 numCaptured     =%d\n", numHDF5Captured);
  }

  return 0;
}
