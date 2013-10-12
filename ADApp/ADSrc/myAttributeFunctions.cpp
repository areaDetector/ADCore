#include <math.h>
#include <string.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include "functAttribute.h"

// These functions demonstrate using user-defined attribute functions

static double _PI = atan(1.0)*4;
static double _E = exp(1.0);

static int myAttrFunct1(const char *paramString, functAttribute *pAttribute)
{
    int ten = 10;
    const char *gettysburg = "Four score and seven years ago our fathers";

    if (!strcmp(paramString, "PI")) {
        pAttribute->setValue(NDAttrFloat64, &_PI);
    }
    else if (!strcmp(paramString, "E")) {
        pAttribute->setValue(NDAttrFloat64, &_E);
    } 
    else if (!strcmp(paramString, "10")) {
        pAttribute->setValue(NDAttrInt32, &ten);
    }
    else if (!strcmp(paramString, "GETTYSBURG")) {
        pAttribute->setValue(NDAttrString, (char *)gettysburg);
    } 
    else {
        return ND_ERROR;
    }
    return ND_SUCCESS;
}

extern "C" {
epicsRegisterFunction(myAttrFunct1);
}
