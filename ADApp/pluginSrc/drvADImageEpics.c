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
static const iocshArg initArg2 = { "imagePort",iocshArgString};
static const iocshArg initArg3 = { "imageAddr",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3};
static const iocshFuncDef initFuncDef = {"drvADImageConfigure",4,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvADImageConfigure(args[0].sval, args[1].ival, args[2].sval, args[3].ival);
}

void ADImageRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(ADImageRegister);
