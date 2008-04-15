/* NDArrayBuffEpics.c
 *
 * EPICS iocsh function to initialize NDArrayBuff library.
 * By making this separate file for the EPICS dependent code the driver itself
 * only needs libCom from EPICS for OS-independence.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  April 6, 2008
 *
 */
 
#include <iocsh.h>
#include <epicsExport.h>

#include "NDArrayBuff.h"


/* Code for iocsh registration */

/* simDetectorConfig */
static const iocshArg initArg0 = {"maxBuffersg", iocshArgInt};
static const iocshArg initArg1 = {"maxMemory", iocshArgInt};
static const iocshArg * const initArgs[2] = {&initArg0,
                                             &initArg1};
static const iocshFuncDef NDArrayBuffInit = {"NDArrayBuffInit", 2, initArgs};
static void NDArrayBuffInitCallFunc(const iocshArgBuf *args)
{
    NDArrayBuff->init(args[0].ival, args[1].ival);
}


static void NDArrayBuffRegister(void)
{

    iocshRegister(&NDArrayBuffInit, NDArrayBuffInitCallFunc);
}

epicsExportRegistrar(NDArrayBuffRegister);


