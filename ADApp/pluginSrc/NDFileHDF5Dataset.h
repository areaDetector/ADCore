#ifndef NDFILEHDF5DATASET_H_
#define NDFILEHDF5DATASET_H_

#include <string>
#include <hdf5.h>
#include "NDPluginFile.h"

/** Class used for writing a Dataset with the NDFileHDF5 plugin.
  */
class NDFileHDF5Dataset
{
  public:
    NDFileHDF5Dataset(asynUser *pAsynUser, const std::string& name, hid_t dataset);

    asynStatus configureDims(NDArray *pArray, bool multiframe, int extradimensions, int *extra_dims, int *user_chunking);
    void extendDataSet(int extradims);
    asynStatus writeFile(NDArray *pArray, hid_t datatype, hid_t dataspace, hsize_t *framesize);
    hid_t getHandle();

  private:

    asynUser    *pAsynUser_;   // Pointer to the asynUser structure
    std::string name_;         // Name of this dataset
    hid_t       dataset_;      // Dataset handle
    int         nextRecord_; 
    int         arrayDims[ND_ARRAY_MAX_DIMS];
    int         rank_;         // number of dimensions
    hsize_t     *dims_;        // Array of current dimension sizes. This updates as various dimensions grow.
    hsize_t     *maxdims_;     // Array of maximum dimension sizes. The value -1 is HDF5 term for infinite.
    hsize_t     *chunkdims_;   // Array of chunk size in each dimension. Only the dimensions that indicate
                               // the frame size (width, height) can really be tweaked. All other dimensions
                               // should be set to 1.
    hsize_t     *offset_;      // Array of current offset in each dimension. The frame dimensions always have
                               // 0 offset but additional dimensions may grow as new frames are added.
    hsize_t     *virtualdims_; // The desired sizes of the extra (virtual) dimensions: {Y, X, n}
    char        *ptrDimensionNames[ND_ARRAY_MAX_DIMS]; // Array of strings with human readable names for each dimension
    char        *dimsreport_;  // A string which contain a verbose report of all dimension sizes. The method getDimsReport fill in this
};


#endif

