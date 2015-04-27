/* NDFileJPEG.cpp
 * Writes NDArrays to JPEG files.
 *
 * Mark Rivers
 * May 9, 2009
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#include <epicsStdio.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginFile.h"
#include "NDFileJPEG.h"


static const char *driverName = "NDFileJPEG";

/** Opens a JPEG file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus NDFileJPEG::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    static const char *functionName = "openFile";
    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute;
    int quality;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: only 8-bit data is supported\n",
            driverName, functionName);
        return(asynError);

    }

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    if (pArray->ndims == 2) {
        this->jpegInfo.image_width  = (JDIMENSION)pArray->dims[0].size;
        this->jpegInfo.image_height = (JDIMENSION)pArray->dims[1].size;
        this->jpegInfo.input_components = 1;
        this->jpegInfo.in_color_space = JCS_GRAYSCALE;
        this->colorMode = NDColorModeMono;
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        this->jpegInfo.image_width  = (JDIMENSION)pArray->dims[1].size;
        this->jpegInfo.image_height = (JDIMENSION)pArray->dims[2].size;
        this->jpegInfo.input_components = 3;
        this->jpegInfo.in_color_space = JCS_RGB;
        this->colorMode = NDColorModeRGB1;
    } else if ((pArray->ndims == 3) && (pArray->dims[1].size == 3) && (colorMode == NDColorModeRGB2)) {
        this->jpegInfo.image_width  = (JDIMENSION)pArray->dims[0].size;
        this->jpegInfo.image_height = (JDIMENSION)pArray->dims[2].size;
        this->jpegInfo.input_components = 3;
        this->jpegInfo.in_color_space = JCS_RGB;
        this->colorMode = NDColorModeRGB2;
    } else if ((pArray->ndims == 3) && (pArray->dims[2].size == 3) && (colorMode == NDColorModeRGB3)) {
        this->jpegInfo.image_width  = (JDIMENSION)pArray->dims[0].size;
        this->jpegInfo.image_height = (JDIMENSION)pArray->dims[1].size;
        this->jpegInfo.input_components = 3;
        this->jpegInfo.in_color_space = JCS_RGB;
        this->colorMode = NDColorModeRGB3;
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: unsupported array structure\n",
            driverName, functionName);
        return(asynError);
    }

   /* Create the file. */
    if ((this->outFile = fopen(fileName, "wb")) == NULL ) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s error opening file %s\n",
        driverName, functionName, fileName);
        return(asynError);
    }
    
    jpeg_set_defaults(&this->jpegInfo);

    /* Set the file quality */
    /* Must lock when accessing parameter library */
    this->lock();
    getIntegerParam(NDFileJPEGQuality, &quality);
    this->unlock();
    jpeg_set_quality(&this->jpegInfo, quality, TRUE);
    
    jpeg_start_compress(&this->jpegInfo, TRUE);
    return(asynSuccess);
}

/** Writes single NDArray to the JPEG file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus NDFileJPEG::writeFile(NDArray *pArray)
{
    JSAMPROW row_pointer[1];
    int nwrite=0;
    unsigned char *pRed=NULL, *pGreen=NULL, *pBlue=NULL, *pData=NULL, *pOut, *buffer=NULL;
    int sizeX = (int)this->jpegInfo.image_width;
    int sizeY = (int)this->jpegInfo.image_height;
    int stepSize=0, i;
    static const char *functionName = "writeFile";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s:%s: %lu, %lu\n", 
              driverName, functionName, (unsigned long)pArray->dims[0].size, (unsigned long)pArray->dims[1].size);

    switch (this->colorMode) {
        case NDColorModeMono:
        case NDColorModeRGB1:
            pData = (unsigned char *)pArray->pData;
            break;
        case NDColorModeRGB2:
            buffer = (unsigned char *)malloc(sizeX * sizeY * 3);
            stepSize = sizeX * 3;
            pRed = (unsigned char *)pArray->pData;
            pGreen = pRed + sizeX;
            pBlue = pGreen + sizeX;
            break;
        case NDColorModeRGB3:
            buffer = (unsigned char *)malloc(sizeX * sizeY * 3);
            stepSize = sizeX;
            pRed = (unsigned char *)pArray->pData;
            pGreen = pRed + sizeX * sizeY;
            pBlue = pGreen + sizeX * sizeY;
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: unknown color mode %d\n",
                driverName, functionName, this->colorMode);
            return(asynError);
            break;
    }
    while ((int)this->jpegInfo.next_scanline < sizeY) {
        switch (this->colorMode) {
            case NDColorModeMono:
            case NDColorModeRGB1:
                row_pointer[0] = pData;
                nwrite = jpeg_write_scanlines(&this->jpegInfo, row_pointer, 1);
                pData += sizeX * this->jpegInfo.input_components;
                break;
            case NDColorModeRGB2:
            case NDColorModeRGB3:
                row_pointer[0] = buffer;
                pOut = buffer;
                for (i=0; i<sizeX; i++) {
                    *pOut++ = pRed[i];
                    *pOut++ = pGreen[i];
                    *pOut++ = pBlue[i];
                }
                nwrite = jpeg_write_scanlines(&this->jpegInfo, row_pointer, 1);
                pRed += stepSize;
                pBlue += stepSize;
                pGreen += stepSize;
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s:%s: unknown color mode %d\n",
                    driverName, functionName, this->colorMode);
                return(asynError);
                break;
        }
        if (nwrite != 1) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: error writing data to file\n",
                driverName, functionName);
            return(asynError);
        }
    }
    if (buffer) free(buffer);

    return(asynSuccess);
}

/** Reads single NDArray from a JPEG file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDFileJPEG::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the JPEG file. */
asynStatus NDFileJPEG::closeFile()
{
    //static const char *functionName = "closeFile";

    jpeg_finish_compress(&this->jpegInfo);
    fclose(this->outFile);

    return asynSuccess;
}

static void init_destination(j_compress_ptr cinfo)
{
    jpegDestMgr *pdest = (jpegDestMgr*) cinfo->dest;
    pdest->pNDFileJPEG->initDestination();
}

/** Initializes the destination file; should be private but called from C so must be public */
void NDFileJPEG::initDestination()
{
    jpegDestMgr *pdest = (jpegDestMgr*) this->jpegInfo.dest;

    pdest->pub.next_output_byte = this->jpegBuffer;
    pdest->pub.free_in_buffer = JPEG_BUF_SIZE;
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
    jpegDestMgr *pdest = (jpegDestMgr*) cinfo->dest;
    return pdest->pNDFileJPEG->emptyOutputBuffer();
}

/** Empties the output buffer; should be private but called from C so must be public */
boolean NDFileJPEG::emptyOutputBuffer()
{
    jpegDestMgr *pdest = (jpegDestMgr*) this->jpegInfo.dest;
    static const char *functionName = "emptyOutputBuffer";

    if (fwrite(this->jpegBuffer, 1, JPEG_BUF_SIZE, this->outFile) !=
      (size_t) JPEG_BUF_SIZE) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error writing JPEG file\n",
            driverName, functionName);
        return FALSE;
    }
    pdest->pub.next_output_byte = this->jpegBuffer;
    pdest->pub.free_in_buffer = JPEG_BUF_SIZE;
    return TRUE;
}

static void term_destination (j_compress_ptr cinfo)
{
    jpegDestMgr *pdest = (jpegDestMgr*) cinfo->dest;
    pdest->pNDFileJPEG->termDestination();
}

/** Terminates the destination file; should be private but called from C so must be public */
void NDFileJPEG::termDestination()
{
    jpegDestMgr *pdest = (jpegDestMgr*) this->jpegInfo.dest;
    size_t datacount = JPEG_BUF_SIZE - pdest->pub.free_in_buffer;
    static const char *functionName = "termDestination";

    /* Write any data remaining in the buffer */
    if (datacount > 0) {
        if (fwrite(this->jpegBuffer, 1, datacount, this->outFile) != datacount)
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s error writing JPEG file\n",
                driverName, functionName);
    }
    fflush(this->outFile);
    /* Make sure we wrote the output file OK */
    if (ferror(this->outFile))
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error flushing JPEG file\n",
            driverName, functionName);
}


/** Constructor for NDFileJPEG; all parameters are simply passed to NDPluginFile::NDPluginFile.
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
NDFileJPEG::NDFileJPEG(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_JPEG_PARAMS,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //static const char *functionName = "NDFileJPEG";

    createParam(NDFileJPEGQualityString, asynParamInt32, &NDFileJPEGQuality);
    
    jpeg_create_compress(&this->jpegInfo);
    this->jpegInfo.err = jpeg_std_error(&this->jpegErr);

    /* Note: we don't use the built-in stdio routines, because this does not work when using
     * the prebuilt library and either VC++ or g++ on Windows.  The FILE pointers are wrong
     * when doing that.  Rather we implement our own jpeg_destination_mgr structure and handle
     * the I/O ourselves.  The code we use is almost a direct copy from jdatadst.c in the standard
     * package. */
    this->destMgr.pub.init_destination = init_destination;
    this->destMgr.pub.empty_output_buffer = empty_output_buffer;
    this->destMgr.pub.term_destination = term_destination;
    this->destMgr.pNDFileJPEG = this;
    this->jpegInfo.dest = (jpeg_destination_mgr *) &this->destMgr;

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileJPEG");
    this->supportsMultipleArrays = 0;
    setIntegerParam(NDFileJPEGQuality, 50);
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileJPEGConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new NDFileJPEG(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDFileJPEGConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileJPEGConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileJPEGRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileJPEGRegister);
}
