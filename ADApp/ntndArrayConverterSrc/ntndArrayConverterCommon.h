#ifndef INC_ntndArrayConverterCommon_H
#define INC_ntndArrayConverterCommon_H
#include <NDArray.h>


typedef struct NTNDArrayInfo
{
    int ndims;
    size_t dims[ND_ARRAY_MAX_DIMS];
    size_t nElements, totalBytes;
    int bytesPerElement;
    NDColorMode_t colorMode;
    NDDataType_t dataType;
    std::string codec;

    struct
    {
        int dim;
        size_t size, stride;
    }x, y, color;
} NTNDArrayInfo_t;

#endif // INC_ntndArrayConverterCommon_H