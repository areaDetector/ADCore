/*
 * drvNDFileEpics.c
 * 
 * EPICS iocsh file for drvNDFile.c.
 *
 * Author: Mark Rivers
 *
 * Created April 5, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iocsh.h>
#include <epicsExport.h>

#include "drvNDFile.h"


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4};
static const iocshFuncDef initFuncDef = {"drvNDFileConfigure",5,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvNDFileConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival);
}

void NDFileRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(NDFileRegister);
