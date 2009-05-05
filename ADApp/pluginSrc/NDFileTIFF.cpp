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

#include "NDPluginFile.h"
#include "NDFileTIFF.h"
#include "drvNDFileTIFF.h"
#include "tiffio.h"

static const char *driverName = "NDFileTIFF";

/* Handle errors by printing an error message and exiting with a
 * non-zero status. */
#define ERR(e) {asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, \
                "%s:%s error=%s\n", \
                driverName, functionName, nc_strerror(e)); \
                return(asynError);}

/* NDArray string attributes can be of any length, but netCDF requires a fixed maximum length
 * which we define here. */
#define MAX_ATTRIBUTE_STRING_SIZE 256

/** This is called to open a TIFF file.
*/
asynStatus NDFileTIFF::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    /* When we create TIFF variables and dimensions, we get back an
     * ID for each one. */
    nc_type ncType=NC_NAT;
    static const char *functionName = "openFile";
	short bitsPerSample, samplesPerPixel;
    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

    /* Set the next record in the file to 0 */
    this->nextRecord = 0;
	printf("FileName: %s\n", fileName);
    /* Create the file. */
    if ((this->output = TIFFOpen(fileName, "w")) == NULL ) {
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "TIFF Plugin: Trouble Opening file %s\n",
		fileName);

        ERR(-1);
	}

    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
			printf ("Open 8 bit\n");
            bitsPerSample = 8;
            break;
        case NDInt16:
        case NDUInt16:
			printf ("Open 16 bit\n");
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
    static const char *functionName = "writeFile";
	int sizex, sizey;
	char ManufacturerString[MAX_ATTRIBUTE_STRING_SIZE];
	char ModelString[MAX_ATTRIBUTE_STRING_SIZE];

    getIntegerParam(ADImageSizeX, &sizex);
	getIntegerParam(ADImageSizeY, &sizey);
	getStringParam(ADManufacturer, MAX_ATTRIBUTE_STRING_SIZE, ManufacturerString);
	getStringParam(ADModel, MAX_ATTRIBUTE_STRING_SIZE, ModelString);

    printf("Hello writeFile: %d, %d, %d, %d\n", sizex, sizey, pArray->dims[0].size, pArray->dims[1].size);
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
			if ( TIFFWriteEncodedStrip( this->output, 0, (signed char *)pArray->pData, stripsize) == 0) {
				asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "TIFF Plugin: Trouble Writing data to file\n");
			}
            break;
        case NDInt16:
        case NDUInt16:
			if ( TIFFWriteEncodedStrip( this->output, 0, (short *)pArray->pData, stripsize) == 0) {
				asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "TIFF Plugin: Trouble Writing data to file\n");
			}
            break;
        case NDInt32:
        case NDUInt32:
			if ( TIFFWriteEncodedStrip( this->output, 0, (long int *)pArray->pData, stripsize) == 0) {
				asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "TIFF Plugin: Trouble Writing data to file\n");
			}
            break;
        case NDFloat32:
			printf( "NDFileTIFF does not support Floats\n");
            break;
        case NDFloat64:
			printf( "NDFileTIFF does not support Floats\n");
            break;
    }

    this->nextRecord++;
    return(asynSuccess);
}

asynStatus NDFileTIFF::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


asynStatus NDFileTIFF::closeFile()
{
    static const char *functionName = "closeFile";

    printf("Hello closeFile\n");
		TIFFClose(this->output);

    return asynSuccess;
}


/* Configuration routine.  Called directly, or from the iocsh function in drvNDFileEpics */

extern "C" int drvNDFileTIFFConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr,
                                     int priority, int stackSize)
{
    new NDFileTIFF(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                     priority, stackSize);
    return(asynSuccess);
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
    this->pAttributeId = NULL;
}

