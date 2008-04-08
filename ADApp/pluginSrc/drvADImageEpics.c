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
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "imagePort",iocshArgString};
static const iocshArg initArg4 = { "imageAddr",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4};
static const iocshFuncDef initFuncDef = {"drvADImageConfigure",5,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvADImageConfigure(args[0].sval, args[1].ival, args[1].ival, args[3].sval, args[4].ival);
}

void ADImageRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(ADImageRegister);
