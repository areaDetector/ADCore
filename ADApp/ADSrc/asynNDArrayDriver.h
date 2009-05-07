#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include "asynPortDriver.h"
#include "NDArray.h"
#include "PVAttributes.h"

/** This is the class from which NDArray drivers are derived; implements the asynGenericPointer functions, assuming
  * that these reference NDArray objects. 
  * For areaDetector, both plugins and detector drivers are indirectly derived from this class.
  * asynNDArrayDriver inherits from asynPortDriver.
  */
class asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize);
    virtual void report(FILE *fp, int details);

    /* These are the methods that are new to this class */
    virtual int createFileName(int maxChars, char *fullFileName);
    virtual int createFileName(int maxChars, char *filePath, char *fileName);
    virtual int readPVAttributesFile(const char *fileName);

protected:
    NDArray **pArrays;             /**< An array of NDArray pointers used to store data in the driver */
    NDArrayPool *pNDArrayPool;     /**< An NDArrayPool object used to allocate and manipulate NDArray objects */
    PVAttributes *pPVAttributes;   /**< A PVAttributes object used to obtain the current values of a set of
                                     *  EPICS PVs */
};

#endif
