/*
 * drvNDStdArraysEpics.c
 * 
 * EPICS iocsh file for drvNDStdArrays.c.
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

#include "drvNDStdArrays.h"


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxMemory",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5};
static const iocshFuncDef initFuncDef = {"drvNDStdArraysConfigure",6,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvNDStdArraysConfigure(args[0].sval, args[1].ival, args[2].ival, 
                            args[3].sval, args[4].ival, args[5].ival);
}

void NDStdArraysRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(NDStdArraysRegister);
