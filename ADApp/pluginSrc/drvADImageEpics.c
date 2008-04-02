/*
 * drvADImageEpics.c
 * 
 * EPICS iocsh file for drvADImage.c.
 *
 * Author: Mark Rivers
 *
 * Created April 2, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iocsh.h>
#include <epicsExport.h>

#include "drvADImage.h"


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "detectorPortName",iocshArgString};
static const iocshArg * const initArgs[2] = {&initArg0,
                                             &initArg1};
static const iocshFuncDef initFuncDef = {"drvADImageConfigure",2,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvADImageConfigure(args[0].sval, args[1].sval);
}

void ADImageRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(ADImageRegister);
