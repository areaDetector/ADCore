#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <registryFunction.h>
#include <epicsTime.h>

#include <asynPortDriver.h>

#include <epicsExport.h>
#include "functAttribute.h"

// These functions demonstrate using user-defined attribute functions

static double _PI = atan(1.0)*4;
static double _E = exp(1.0);

typedef enum {
    functPi,
    functE,
    functTen,
    functGettysburg,
    functTime64,
    functInt8,
    functUInt8,
    functInt16,
    functUInt16,
    functInt32,
    functUInt32,
    functInt64,
    functUInt64,
    functFloat32,
    functFloat64
} myFunct_t;

static int myAttrFunct1(const char *paramString, void **functionPvt, functAttribute *pAttribute)
{
    int ten = 10;
    const char *gettysburg = "Four score and seven years ago our fathers";
    int *paramIndex;

    paramIndex = (int *)*functionPvt;

    if (paramIndex == 0) {
        paramIndex = (int *)malloc(sizeof(int));
        // This is the first time we have been called for this attribute.
        // Convert paramString to paramIndex

        if (!strcmp(paramString, "PI")) {
            pAttribute->setDataType(NDAttrFloat64);
            *paramIndex = functPi;
        }
        else if (!strcmp(paramString, "E")) {
            pAttribute->setDataType(NDAttrFloat64);
            *paramIndex = functE;
        }
        else if (!strcmp(paramString, "10")) {
            pAttribute->setDataType(NDAttrInt32);
            *paramIndex = functTen;
        }
        else if (!strcmp(paramString, "GETTYSBURG")) {
            pAttribute->setDataType(NDAttrString);
            *paramIndex = functGettysburg;
        }
        else if (!strcmp(paramString, "TIME64")) {
            pAttribute->setDataType(NDAttrUInt64);
            *paramIndex = functTime64;
        }
        else if (!strcmp(paramString, "INT8")) {
            pAttribute->setDataType(NDAttrInt8);
            *paramIndex = functInt8;
        }
        else if (!strcmp(paramString, "UINT8")) {
            pAttribute->setDataType(NDAttrUInt8);
            *paramIndex = functUInt8;
        }
        else if (!strcmp(paramString, "INT16")) {
            pAttribute->setDataType(NDAttrInt16);
            *paramIndex = functInt16;
        }
        else if (!strcmp(paramString, "UINT16")) {
            pAttribute->setDataType(NDAttrUInt16);
            *paramIndex = functUInt16;
        }
        else if (!strcmp(paramString, "INT32")) {
            pAttribute->setDataType(NDAttrInt32);
            *paramIndex = functInt32;
        }
        else if (!strcmp(paramString, "UINT32")) {
            pAttribute->setDataType(NDAttrUInt32);
            *paramIndex = functUInt32;
        }
        else if (!strcmp(paramString, "INT64")) {
            pAttribute->setDataType(NDAttrInt64);
            *paramIndex = functInt64;
        }
        else if (!strcmp(paramString, "UINT64")) {
            pAttribute->setDataType(NDAttrUInt64);
            *paramIndex = functUInt64;
        }
        else if (!strcmp(paramString, "FLOAT32")) {
            pAttribute->setDataType(NDAttrFloat32);
            *paramIndex = functFloat32;
        }
        else if (!strcmp(paramString, "FLOAT64")) {
            pAttribute->setDataType(NDAttrFloat64);
            *paramIndex = functFloat64;
        }
        else {
            printf("Error, unknown parameter string = %s\n", paramString);
            free(paramIndex);
            paramIndex = 0;
            return ND_ERROR;
        }
        *functionPvt = paramIndex;
    }

    switch (*paramIndex) {
        case functPi:
            pAttribute->setValue(&_PI);
            break;

        case functE:
            pAttribute->setValue(&_E);
            break;

        case functTen:
            pAttribute->setValue(&ten);
            break;

        case functGettysburg:
            pAttribute->setValue((char *)gettysburg);
            break;

        case functTime64: {
            epicsTimeStamp now;
            epicsTimeGetCurrent(&now);
            epicsUInt64 value = now.secPastEpoch;
            value = (value << 32) | now.nsec;
            pAttribute->setValue(&value);
            break;
          }

        case functInt8: {
            epicsInt8 val=-8;
            pAttribute->setValue(&val);
            break;
          }

        case functUInt8: {
            epicsUInt8 val=8;
            pAttribute->setValue(&val);
            break;
          }

        case functInt16: {
            epicsInt16 val=-16;
            pAttribute->setValue(&val);
            break;
          }

        case functUInt16: {
            epicsUInt16 val=16;
            pAttribute->setValue(&val);
            break;
          }

        case functInt32: {
            epicsInt32 val=-32;
            pAttribute->setValue(&val);
            break;
          }

        case functUInt32: {
            epicsUInt32 val=32;
            pAttribute->setValue(&val);
            break;
          }

        case functInt64: {
            epicsInt64 val=-64;
            pAttribute->setValue(&val);
            break;
          }

        case functUInt64: {
            epicsUInt64 val=64;
            pAttribute->setValue(&val);
            break;
          }

        case functFloat32: {
            epicsFloat32 val=32.0;
            pAttribute->setValue(&val);
            break;
          }

        case functFloat64: {
            epicsFloat64 val=64.0;
            pAttribute->setValue(&val);
            break;
          }

        default:
            return ND_ERROR;
    }
    return ND_SUCCESS;
}

extern "C" {
epicsRegisterFunction(myAttrFunct1);
}
