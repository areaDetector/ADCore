#include <asynDriver.h>
#include <epicsThread.h>

#include "PVAttributes.h"

int main(int argc, char *argv[]) 
{
    PVAttributes *pAttr;
    NDArray *pArray;
    int i;

    pasynTrace->setTraceMask(NULL, 255);
    
    pAttr = new PVAttributes();
    pArray = new NDArray();
    
    pAttr->addPV("13SIM1:cam1:Acquire",          "Camera acquiring", DBR_NATIVE);
    pAttr->addPV("13SIM1:cam1:AcquireTime",      "Camera acquire time", DBR_NATIVE);
    pAttr->addPV("13SIM1:cam1:ArrayCallbacks",   "Array callbacks", DBR_NATIVE);
    pAttr->addPV("13SIM1:cam1:ColorMode",        "Color mode", DBR_STRING);
    pAttr->addPV("13SIM1:cam1:ImageCounter_RBV", "Image counter", DBR_FLOAT);
    pAttr->addPV("13SIM1:image1:ArrayData",      "First pixel of image data", DBR_NATIVE);
    pAttr->addPV("13SIM1:file1:PortName_RBV",    "Port name", DBR_NATIVE);
    pAttr->addPV("13SIM1:file1:FilePath_RBV",    "File path", DBR_STRING);
    
    /* Sleep for 1 second to let the connection and value allbacks run */
    epicsThreadSleep(1.0);

    for (i=0; i<3; i++) {   
        pAttr->getValues(pArray);
        pAttr->report(20);
        pArray->report(20);
        epicsThreadSleep(5.0);
    }
    printf("\nRemoving FilePath and ImageCounter_RBV\n");
    pAttr->removePV("13SIM1:file1:FilePath_RBV");
    pAttr->removePV("13SIM1:cam1:ImageCounter_RBV");
    pAttr->getValues(pArray);
    pAttr->report(20);

    printf("\nClearing all PVs\n");
    pAttr->clearPVs();
    pAttr->getValues(pArray);
    pAttr->report(20);

    printf("\nAdding Acquire\n");
    pAttr->addPV("13SIM1:cam1:Acquire",    "File path", DBR_STRING);
    /* Sleep for 1 second to let the connection and value allbacks run */
    epicsThreadSleep(1.0);
    pAttr->getValues(pArray);
    pAttr->report(20);
    
    printf("\nDeleting pAttr\n");
    delete pAttr;

    return(0);
    
}
