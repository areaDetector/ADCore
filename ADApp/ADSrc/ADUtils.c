/* ADUtils.h
 *
 * This file provides some utility functions that can be used by area detector
 * drivers.
 *
 * Mark Rivers
 * University of Chicago
 * March 5, 2008
 *
 */

#include <string.h>
#include <stdio.h>
#include "ADUtils.h"
#include "ADParamLib.h"
#include "ADInterface.h"

/* The following macros save an enormous amount of code when converting data types */

/* This macro converts the data type of 2 images that have the same dimensions */

#define CONVERT_TYPE(DATA_TYPE_IN, DATA_TYPE_OUT) {  \
    int i;                                           \
    DATA_TYPE_IN *pIn = (DATA_TYPE_IN *)imageIn;     \
    DATA_TYPE_OUT *pOut = (DATA_TYPE_OUT *)imageOut; \
    for (i=0; i<sizeXIn*sizeYIn; i++) {              \
        *pOut++ = (DATA_TYPE_OUT)(*pIn++);           \
    }                                                \
}

#define CONVERT_TYPE_SWITCH(DATA_TYPE_OUT) { \
    switch(dataTypeIn) {                        \
        case ADInt8:                              \
            CONVERT_TYPE(epicsInt8, DATA_TYPE_OUT);     \
            break;                                \
        case ADUInt8:                             \
            CONVERT_TYPE(epicsUInt8, DATA_TYPE_OUT);    \
            break;                                \
        case ADInt16:                             \
            CONVERT_TYPE(epicsInt16, DATA_TYPE_OUT);    \
            break;                                \
        case ADUInt16:                            \
            CONVERT_TYPE(epicsUInt16, DATA_TYPE_OUT);   \
            break;                                \
        case ADInt32:                             \
            CONVERT_TYPE(epicsInt32, DATA_TYPE_OUT);    \
            break;                                \
        case ADUInt32:                            \
            CONVERT_TYPE(epicsUInt32, DATA_TYPE_OUT);   \
            break;                                \
        case ADFloat32:                           \
            CONVERT_TYPE(epicsFloat32, DATA_TYPE_OUT);  \
            break;                                \
        case ADFloat64:                           \
            CONVERT_TYPE(epicsFloat64, DATA_TYPE_OUT);  \
            break;                                \
        default:                                  \
            status = AREA_DETECTOR_ERROR;         \
            break;                                \
    }                                             \
}

#define CONVERT_TYPE_ALL() { \
    switch(dataTypeOut) {                        \
        case ADInt8:                              \
            CONVERT_TYPE_SWITCH(epicsInt8);     \
            break;                                \
        case ADUInt8:                             \
            CONVERT_TYPE_SWITCH(epicsUInt8);    \
            break;                                \
        case ADInt16:                             \
            CONVERT_TYPE_SWITCH(epicsInt16);    \
            break;                                \
        case ADUInt16:                            \
            CONVERT_TYPE_SWITCH(epicsUInt16);   \
            break;                                \
        case ADInt32:                             \
            CONVERT_TYPE_SWITCH(epicsInt32);    \
            break;                                \
        case ADUInt32:                            \
            CONVERT_TYPE_SWITCH(epicsUInt32);   \
            break;                                \
        case ADFloat32:                           \
            CONVERT_TYPE_SWITCH(epicsFloat32);  \
            break;                                \
        case ADFloat64:                           \
            CONVERT_TYPE_SWITCH(epicsFloat64);  \
            break;                                \
        default:                                  \
            status = AREA_DETECTOR_ERROR;         \
            break;                                \
    }                                             \
}


/* This macro computes an output image from an input image, selecting a region of interest
 * with binning and data type conversion. */
#define CONVERT_REGION(DATA_TYPE_IN, DATA_TYPE_OUT) {   \
    int xb, yb, inRow=startY, outRow, outCol;             \
    DATA_TYPE_OUT *pOut, *pOutTemp;                     \
    DATA_TYPE_IN *pIn;                                  \
    for (outRow=0; outRow<sizeYOut; outRow++) {         \
        pOut = (DATA_TYPE_OUT *)imageOut;               \
        pOut += outRow * sizeXOut;                      \
        memset(pOut, 0, sizeXOut*sizeof(DATA_TYPE_OUT));\
        for (yb=0; yb<binY; yb++) {                     \
            pOutTemp = pOut;                            \
            pIn = (DATA_TYPE_IN *)imageIn;              \
            pIn += inRow*sizeXIn + startX;              \
            for (outCol=0; outCol<sizeXIn; outCol++) {  \
                *pOutTemp += (DATA_TYPE_OUT)*pIn++;     \
                for (xb=1; xb<binX; xb++) {             \
                    *pOutTemp += (DATA_TYPE_OUT)*pIn++; \
                } /* Next xbin */                       \
                pOutTemp++;                             \
            } /* Next outCol */                         \
            inRow++;                                    \
        } /* Next ybin */                               \
    } /* Next outRow */                                 \
}

#define CONVERT_REGION_SWITCH(DATA_TYPE_OUT) { \
    switch(dataTypeIn) {                        \
        case ADInt8:                              \
            CONVERT_REGION(epicsInt8, DATA_TYPE_OUT);     \
            break;                                \
        case ADUInt8:                             \
            CONVERT_REGION(epicsUInt8, DATA_TYPE_OUT);    \
            break;                                \
        case ADInt16:                             \
            CONVERT_REGION(epicsInt16, DATA_TYPE_OUT);    \
            break;                                \
        case ADUInt16:                            \
            CONVERT_REGION(epicsUInt16, DATA_TYPE_OUT);   \
            break;                                \
        case ADInt32:                             \
            CONVERT_REGION(epicsInt32, DATA_TYPE_OUT);    \
            break;                                \
        case ADUInt32:                            \
            CONVERT_REGION(epicsUInt32, DATA_TYPE_OUT);   \
            break;                                \
        case ADFloat32:                           \
            CONVERT_REGION(epicsFloat32, DATA_TYPE_OUT);  \
            break;                                \
        case ADFloat64:                           \
            CONVERT_REGION(epicsFloat64, DATA_TYPE_OUT);  \
            break;                                \
        default:                                  \
            status = AREA_DETECTOR_ERROR;         \
            break;                                \
    }                                             \
}

#define CONVERT_REGION_ALL() { \
    switch(dataTypeOut) {                        \
        case ADInt8:                              \
            CONVERT_REGION_SWITCH(epicsInt8);     \
            break;                                \
        case ADUInt8:                             \
            CONVERT_REGION_SWITCH(epicsUInt8);    \
            break;                                \
        case ADInt16:                             \
            CONVERT_REGION_SWITCH(epicsInt16);    \
            break;                                \
        case ADUInt16:                            \
            CONVERT_REGION_SWITCH(epicsUInt16);   \
            break;                                \
        case ADInt32:                             \
            CONVERT_REGION_SWITCH(epicsInt32);    \
            break;                                \
        case ADUInt32:                            \
            CONVERT_REGION_SWITCH(epicsUInt32);   \
            break;                                \
        case ADFloat32:                           \
            CONVERT_REGION_SWITCH(epicsFloat32);  \
            break;                                \
        case ADFloat64:                           \
            CONVERT_REGION_SWITCH(epicsFloat64);  \
            break;                                \
        default:                                  \
            status = AREA_DETECTOR_ERROR;         \
            break;                                \
    }                                             \
}

static int setParamDefaults( void *params)
{
    /* Set some reasonable defaults for some parameters */
    int status = AREA_DETECTOR_OK;
    
    status |= ADParam->setString (params, ADManufacturer, "Unknown");
    status |= ADParam->setString (params, ADModel,        "Unknown");
    status |= ADParam->setInteger(params, ADMinY,         0);
    status |= ADParam->setInteger(params, ADMinX,         0);
    status |= ADParam->setInteger(params, ADMinY,         0);
    status |= ADParam->setInteger(params, ADBinX,         1);
    status |= ADParam->setInteger(params, ADBinY,         1);
    status |= ADParam->setDouble( params, ADGain,         1.0);
    status |= ADParam->setInteger(params, ADStatus,       ADStatusIdle);
    status |= ADParam->setInteger(params, ADAcquire,      0);
    status |= ADParam->setDouble (params, ADAcquireTime,  1.0);
    status |= ADParam->setInteger(params, ADNumExposures, 1);
    status |= ADParam->setInteger(params, ADNumFrames,    1);
    status |= ADParam->setInteger(params, ADFrameMode,    ADSingleFrame);
    status |= ADParam->setInteger(params, ADFileNumber,   1);
    status |= ADParam->setInteger(params, ADAutoIncrement,1);
    status |= ADParam->setInteger(params, ADAutoSave,     0);
    status |= ADParam->setInteger(params, ADWriteFile,    0);
    status |= ADParam->setInteger(params, ADReadFile,     0);
    status |= ADParam->setInteger(params, ADFileFormat,   0);

    return status;
}

static int bytesPerPixel(ADDataType_t dataType, int *size)
{
    int status = AREA_DETECTOR_OK;
    
    switch(dataType) {
        case ADInt8:
            *size = sizeof(epicsInt8);
            break;
        case ADUInt8:
            *size = sizeof(epicsUInt8);
            break;
        case ADInt16:
            *size = sizeof(epicsInt16);
            break;
        case ADUInt16:
            *size = sizeof(epicsUInt16);
            break;
        case ADInt32:
            *size = sizeof(epicsInt32);
            break;
        case ADUInt32:
            *size = sizeof(epicsUInt32);
            break;
        case ADFloat32:
            *size = sizeof(epicsFloat32);
            break;
        case ADFloat64:
            *size = sizeof(epicsFloat64);
            break;
        default:
            *size = 0;
            status = AREA_DETECTOR_ERROR;
            break;
    }
    return(status);
}

static int convertImage(void *imageIn, 
                        int dataTypeIn,
                        int sizeXIn, int sizeYIn,
                        void *imageOut,
                        int dataTypeOut,
                        int binX, int binY,
                        int startX, int startY,
                        int regionSizeX, int regionSizeY,
                        int *psizeXOut, int *psizeYOut)
{
    int dimsUnchanged, dataSize;
    int sizeXOut = regionSizeX/binX;
    int sizeYOut = regionSizeY/binY;
    int status = AREA_DETECTOR_OK;
    
    *psizeXOut = sizeXOut;
    *psizeYOut = sizeYOut;
       
    /* See if the image dimensions are not to change */
    dimsUnchanged = ((sizeXIn == regionSizeX) &&
                     (sizeYIn == regionSizeY) &&
                     (startX == 0) &&
                     (startY == 0) &&
                     (binX == 1) &&
                     (binY == 1));
 
    if (dimsUnchanged) {
        if (dataTypeIn == dataTypeOut) {
            status = bytesPerPixel(dataTypeIn, &dataSize);
            if (status) return(status);
            /* The dimensions are the same and the data type is the same, 
            * then just copy the input image to the output image */
            memcpy(imageOut, imageIn, sizeXIn*sizeYIn*dataSize);
            return AREA_DETECTOR_OK;
        } else {
            /* We need to convert data types */
            CONVERT_TYPE_ALL();
        }
    } else {
       /* The input and output dimensions are not the same, so we are extracting a region
        * and/or binning */
        CONVERT_REGION_ALL();
    }
                    
    return AREA_DETECTOR_OK;
}


static ADUtilsSupport utilsSupport =
{
  setParamDefaults,
  bytesPerPixel,
  convertImage
};

ADUtilsSupport *ADUtils = &utilsSupport;
