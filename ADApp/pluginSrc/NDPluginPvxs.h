#ifndef NDPluginPvxs_H
#define NDPluginPvxs_H


#include "NDPluginDriver.h"
#include <vector>

#define NDPluginPvxsPvNameString "PV_NAME"

class NTNDArrayRecordPvxs;
typedef std::shared_ptr<NTNDArrayRecordPvxs> NTNDArrayRecordPvxsPtr;

/** Converts NDArray callback data into EPICS V4 NTNDArray data and exposes it
  * as an EPICS V4 PV  */
class NDPLUGIN_API NDPluginPvxs : public NDPluginDriver,
                     public std::enable_shared_from_this<NDPluginPvxs>
{
public:
    NDPluginPvxs(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr, const char *pvName,
                 int maxBuffers, size_t maxMemory, int priority, int stackSize);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

protected:
    int NDPluginPvxsPvName;

private:
    NTNDArrayRecordPvxsPtr m_record;
};

#endif
