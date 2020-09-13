/* NDFileTIFFS3.cpp
 * Writes NDArrays to TIFF files on S3 Storage
 *
 * Stuart B. Wilkins 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginFile.h"
#include "NDFileTIFFS3.h"


/** Constructor for NDFileTIFFS3; all parameters are simply passed to NDPluginFile::NDPluginFile.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDFileTIFFS3::NDFileTIFFS3(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDFileTIFF(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, priority, stackSize)
{
    //static const char *functionName = "NDFileTIFF";

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileTIFFS3");
    this->supportsMultipleArrays = 0;

}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileTIFFS3Configure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr,
                                     int priority, int stackSize)
{
    // Stack size must be a minimum of 40000 on vxWorks because of automatic variables in NDFileTIFF::openFile()
    #ifdef vxWorks
        if (stackSize < 40000) stackSize = 40000;
    #endif
    NDFileTIFFS3 *pPlugin = new NDFileTIFFS3(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                             priority, stackSize);
    return pPlugin->start();
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileTIFFS3Configure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileTIFFS3Configure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileTIFFS3Register(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileTIFFS3Register);
}
