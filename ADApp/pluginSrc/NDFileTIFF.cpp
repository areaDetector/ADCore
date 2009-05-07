/* NDFileTIFF.cpp
 * Writes NDArrays to TIFF files.
 *
 * John Hammonds
 * April 17, 2008
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#include <epicsStdio.h>
#include <epicsExport.h>
#include <iocsh.h>

#include "NDPluginFile.h"
#include "NDFileTIFF.h"
#include "tiffio.h"

static const char *driverName = "NDFileTIFF";

/* NDArray string attributes can be of any length, but netCDF requires a fixed maximum length
 * which we define here. */
#define MAX_ATTRIBUTE_STRING_SIZE 256

/** This is called to open a TIFF file.
*/
asynStatus NDFileTIFF::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    /* When we create TIFF variables and dimensions, we get back an
     * ID for each one. */
    static const char *functionName = "openFile";
    short bitsPerSample=0, samplesPerPixel;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

   /* Create the file. */
    if ((this->output = TIFFOpen(fileName, "w")) == NULL ) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s error opening file %s\n",
        driverName, functionName, fileName);
        return(asynError);
    }

    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            bitsPerSample = 8;
            break;
        case NDInt16:
        case NDUInt16:
            bitsPerSample = 16;
            break;
        case NDInt32:
        case NDUInt32:
            bitsPerSample = 32;
            break;
        case NDFloat32:
            bitsPerSample = 8;
            break;
        case NDFloat64:
            bitsPerSample = 8;
            break;
    }
    samplesPerPixel = 1;
    TIFFSetField(this->output, TIFFTAG_BITSPERSAMPLE, bitsPerSample);
    TIFFSetField(this->output, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
    TIFFSetField (this->output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField (this->output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    return(asynSuccess);
}

/** This is called to write data a single NDArray to the file.  It can be called multiple times
 *  to add arrays to a single file in stream or capture mode */
asynStatus NDFileTIFF::writeFile(NDArray *pArray)
{
    unsigned long int stripsize;
    tsize_t nwrite=0;
    static const char *functionName = "writeFile";
    int sizex, sizey;
    char ManufacturerString[MAX_ATTRIBUTE_STRING_SIZE] = "Unknown";
    char ModelString[MAX_ATTRIBUTE_STRING_SIZE] = "Unknown";

    getIntegerParam(NDArraySizeX, &sizex);
    getIntegerParam(NDArraySizeY, &sizey);
    /* We cannot get the manufacturer and model directly, we don't have access to the parameter
     * library of the driver.  We would have to get this from attributes */
    //getStringParam(ADManufacturer, MAX_ATTRIBUTE_STRING_SIZE, ManufacturerString);
    //getStringParam(ADModel, MAX_ATTRIBUTE_STRING_SIZE, ModelString);

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s:s: %d, %d, %d, %d\n", 
              driverName, functionName, sizex, sizey, pArray->dims[0].size, pArray->dims[1].size);
    sizex = pArray->dims[0].size;
    sizey = pArray->dims[1].size;

    TIFFSetField(this->output, TIFFTAG_IMAGEWIDTH, sizex);
    TIFFSetField(this->output, TIFFTAG_IMAGELENGTH, sizey);
    TIFFSetField (this->output, TIFFTAG_ROWSPERSTRIP, sizey);  //save entire image in one strip.
    TIFFSetField(this->output, TIFFTAG_MAKE, ManufacturerString);
    TIFFSetField(this->output, TIFFTAG_MODEL, ModelString);

    stripsize = TIFFStripSize(this->output);

    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            nwrite = TIFFWriteEncodedStrip( this->output, 0, (signed char *)pArray->pData, stripsize); 
            break;
        case NDInt16:
        case NDUInt16:
            nwrite = TIFFWriteEncodedStrip( this->output, 0, (short *)pArray->pData, stripsize);
            break;
        case NDInt32:
        case NDUInt32:
            nwrite = TIFFWriteEncodedStrip( this->output, 0, (long int *)pArray->pData, stripsize);
            break;
        case NDFloat32:
        case NDFloat64:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: floating point data not yet supported\n",
                driverName, functionName);
            return(asynError);
            break;
    }
    if (nwrite <= 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: error writing data to file\n",
            driverName, functionName);
        return(asynError);
    }

    return(asynSuccess);
}

asynStatus NDFileTIFF::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


asynStatus NDFileTIFF::closeFile()
{
    //static const char *functionName = "closeFile";

    TIFFClose(this->output);

    return asynSuccess;
}


/* The constructor for this class */
NDFileTIFF::NDFileTIFF(const char *portName, int queueSize, int blockingCallbacks,
                           const char *NDArrayPort, int NDArrayAddr,
                           int priority, int stackSize)
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                  NDArrayPort, NDArrayAddr, priority, stackSize)
{
    //const char *functionName = "NDFileTIFF";

    this->supportsMultipleArrays = 0;
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileTIFFConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new NDFileTIFF(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                     priority, stackSize);
    return(asynSuccess);
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
static const iocshFuncDef initFuncDef = {"NDFileTIFFConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileTIFFConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileTIFFRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}
epicsExportRegistrar(NDFileTIFFRegister);
