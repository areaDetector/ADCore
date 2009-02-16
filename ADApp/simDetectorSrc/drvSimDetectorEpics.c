/* drvSimDetectorEpics.c
 *
 * This is the EPICS dependent code for the driver for a simulated area detector.
 * By making this separate file for the EPICS dependent code the driver itself
 * only needs libCom from EPICS for OS-independence.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */
 
#include <iocsh.h>
#include <drvSup.h>
#include <epicsExport.h>

#include "drvSimDetector.h"


/* Code for iocsh registration */

/* simDetectorConfig */
static const iocshArg simDetectorConfigArg0 = {"Port name", iocshArgString};
static const iocshArg simDetectorConfigArg1 = {"Max X size", iocshArgInt};
static const iocshArg simDetectorConfigArg2 = {"Max Y size", iocshArgInt};
static const iocshArg simDetectorConfigArg3 = {"Data type", iocshArgInt};
static const iocshArg simDetectorConfigArg4 = {"maxBuffers", iocshArgInt};
static const iocshArg simDetectorConfigArg5 = {"maxMemory", iocshArgInt};
static const iocshArg simDetectorConfigArg6 = {"priority", iocshArgInt};
static const iocshArg simDetectorConfigArg7 = {"stackSize", iocshArgInt};
static const iocshArg * const simDetectorConfigArgs[] =  {&simDetectorConfigArg0,
                                                          &simDetectorConfigArg1,
                                                          &simDetectorConfigArg2,
                                                          &simDetectorConfigArg3,
                                                          &simDetectorConfigArg4,
                                                          &simDetectorConfigArg5,
                                                          &simDetectorConfigArg6,
                                                          &simDetectorConfigArg7};
static const iocshFuncDef configsimDetector = {"simDetectorConfig", 8, simDetectorConfigArgs};
static void configsimDetectorCallFunc(const iocshArgBuf *args)
{
    simDetectorConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival,
                      args[4].ival, args[5].ival, args[6].ival, args[7].ival);
}


static void simDetectorRegister(void)
{

    iocshRegister(&configsimDetector, configsimDetectorCallFunc);
}

epicsExportRegistrar(simDetectorRegister);


