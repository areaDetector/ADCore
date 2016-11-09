#ifndef NDPluginPva_H
#define NDPluginPva_H

#include <epicsTypes.h>
#include <epicsMutex.h>

#include "NDPluginDriver.h"

#include <pv/serverContext.h>
#include <pv/lock.h>
#include <pv/pvData.h>
#include <vector>

#define NDPluginPvaPvNameString "PV_NAME"

class NTNDArrayRecord;
typedef std::tr1::shared_ptr<NTNDArrayRecord> NTNDArrayRecordPtr;

/** Converts NDArray callback data into EPICS V4 NTNDArray data and exposes it
  * as an EPICS V4 PV  */
class NDPluginPva : public NDPluginDriver,
                     public std::tr1::enable_shared_from_this<NDPluginPva>
{
public:
    POINTER_DEFINITIONS(NDPluginPva);
    NDPluginPva(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr, const char *pvName,
                 size_t maxMemory, int priority, int stackSize);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

protected:
    int NDPluginPvaPvName;

private:
    NTNDArrayRecordPtr m_record;
};

#endif
