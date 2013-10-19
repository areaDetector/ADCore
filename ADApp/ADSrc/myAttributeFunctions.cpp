#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include "functAttribute.h"

// These functions demonstrate using user-defined attribute functions

static double _PI = atan(1.0)*4;
static double _E = exp(1.0);

typedef enum {
    functPi,
    functE,
    functTen,
    functGettysburg
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
        else {
            printf("Error, unknown parameter string = %s\n", paramString);
            *paramIndex = functPi;
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
            
        default:
            return ND_ERROR;
    }
    return ND_SUCCESS;
}

extern "C" {
epicsRegisterFunction(myAttrFunct1);
}
