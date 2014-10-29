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
#include "simDetector.h"
#include <epicsExport.h>

int main(int argc, char **argv)
{
   int numImages;
   int queueSize;
   asynStatus status;
   
   simDetector *simD = new simDetector("SIM1", 1024, 1024, NDUInt8, 0, 0, 0, 0);
   NDPluginStats *stats = new NDPluginStats("STATS1", 20, 0, "SIM1", 0, 0, 0, 0, 0);

   asynInt32Client *numImagesClient = new asynInt32Client("SIM1", 0, ADNumImagesString, 1.0);
   asynInt32Client *queueSizeClient = new asynInt32Client("STATS1", 0, NDPluginDriverQueueSizeString, 1.0);

   status = numImagesClient->write(100);
   status = numImagesClient->read(&numImages);
   status = queueSizeClient->read(&queueSize);

   printf("Read back numImages from driver=%d\n", numImages);
   printf("Read back queue size from plugin=%d\n", queueSize);
   
   numImagesClient->report(stdout, 0);
   queueSizeClient->report(stdout, 0);

   return(0);
}

