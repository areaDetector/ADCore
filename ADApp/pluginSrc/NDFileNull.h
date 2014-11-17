/*
 * NDFileNull.h
 * Dummy file writer, whose main purpose is to allow deleting original driver files without re-writing them in 
 * an actual file plugin.
 *
 * Mark Rivers
 * November 30, 2011
 */

#ifndef DRV_NDFileNULL_H
#define DRV_NDFileNULL_H

#include "NDPluginFile.h"

/** Writes NDArrays in the Null file format. */

class epicsShareClass NDFileNull : public NDPluginFile {
public:
    NDFileNull(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
};
#define NUM_NDFILE_NULL_PARAMS 0
#endif
