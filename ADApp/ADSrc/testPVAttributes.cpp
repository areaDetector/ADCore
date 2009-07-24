#include <asynDriver.h>
#include <epicsThread.h>

#include "PVAttribute.h"

int main(int argc, char *argv[]) 
{
    PVAttributeList *pAttr;
    NDArray *pArray;
    int i;

    pasynTrace->setTraceMask(NULL, 255);
    
    pAttr = new PVAttributeList();
    pArray = new NDArray();
    
    pAttr->addPV("Acquire",        "Camera acquiring",          "13SIM1:cam1:Acquire",          DBR_NATIVE);
    pAttr->addPV("AcquireTime",    "Camera acquire time",       "13SIM1:cam1:AcquireTime",      DBR_NATIVE);
    pAttr->addPV("ArrayCallbacks", "Array callbacks",           "13SIM1:cam1:ArrayCallbacks",   DBR_NATIVE);
    pAttr->addPV("ColorMode",      "Color mode",                "13SIM1:cam1:ColorMode",        DBR_STRING);
    pAttr->addPV("ImageCounter",   "Image counter",             "13SIM1:image1:ArrayData",      DBR_FLOAT);
    pAttr->addPV("ArrayData",      "First pixel of image data", "13SIM1:image1:ArrayData",      DBR_NATIVE);
    pAttr->addPV("PortName",       "Port name",                 "13SIM1:netCDF1:PortName_RBV",  DBR_NATIVE);
    pAttr->addPV("FilePath",       "File path",                 "13SIM1:netCDF1:FilePath_RBV",  DBR_STRING);
    
    /* Sleep for 1 second to let the connection and value allbacks run */
    epicsThreadSleep(1.0);

    for (i=0; i<3; i++) {   
        pAttr->getValues(pArray);
        pAttr->report(20);
        pArray->report(20);
        epicsThreadSleep(5.0);
    }
    printf("\nRemoving FilePath and ImageCounter\n");
    pAttr->removeAttribute("FilePath");
    pAttr->removeAttribute("ImageCounter");
    pAttr->getValues(pArray);
    pAttr->report(20);

    printf("\nClearing all PVs\n");
    pAttr->clearAttributes();
    pAttr->getValues(pArray);
    pAttr->report(20);

    printf("\nAdding Acquire\n");
    pAttr->addPV("Acquire",         "Camera acquiring",         "13SIM1:cam1:Acquire",          DBR_STRING);
    /* Sleep for 1 second to let the connection and value allbacks run */
    epicsThreadSleep(1.0);
    pAttr->getValues(pArray);
    pAttr->report(20);
    
    printf("\nDeleting pAttr\n");
    delete pAttr;

    return(0);
    
}
