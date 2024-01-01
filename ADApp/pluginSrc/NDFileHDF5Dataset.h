#ifndef NDFILEHDF5DATASET_H_
#define NDFILEHDF5DATASET_H_

#include <string>
#include <hdf5.h>
#include <NDPluginAPI.h>
#include "NDPluginFile.h"
#include "NDFileHDF5VersionCheck.h"

/** Class used for writing a Dataset with the NDFileHDF5 plugin.
  */
class NDPLUGIN_API NDFileHDF5Dataset
{
  public:
    NDFileHDF5Dataset(asynUser *pAsynUser, const std::string& name, hid_t dataset);
    virtual ~NDFileHDF5Dataset();

    asynStatus configureDims(NDArray *pArray, bool multiframe, int extradimensions, int *extra_dims, int *extra_dim_chunking, int *user_chunking);
    asynStatus extendDataSet(int extradims);
    asynStatus extendDataSet(int extradims, hsize_t *offsets);
    asynStatus verifyChunking(NDArray *pArray);
    void configureCompression(Codec_t codec);
    asynStatus writeFile(NDArray *pArray, hid_t datatype, hid_t dataspace, hsize_t *framesize);
    hid_t getHandle();
    asynStatus flushDataset();
    hsize_t getDim(int index);
    hsize_t getMaxDim(int index);
    hsize_t getOffset(int index);
    hsize_t getVirtualDim(int index);

  private:

    asynUser    *pAsynUser_;   // Pointer to the asynUser structure
    std::string name_;         // Name of this dataset
    hid_t       dataset_;      // Dataset handle
    int         nextRecord_;
    bool        multiFrame_;   // Whether this is a multi frame dataset
    int         rank_;         // number of dimensions
    int         extra_rank_;   // number of extra dimensions
    hsize_t     *dims_;        // Array of current dimension sizes. This updates as various dimensions grow.
    hsize_t     *chunkdims_;   // Array of current configured chunk dimension sizes.
    hsize_t     *maxdims_;     // Array of maximum dimension sizes. The value -1 is HDF5 term for infinite.
    hsize_t     *offset_;      // Array of current offset in each dimension. The frame dimensions always have
                               // 0 offset but additional dimensions may grow as new frames are added.
    hsize_t     *virtualdims_; // The desired sizes of the extra (virtual) dimensions: {Y, X, n}
    hsize_t     *virtualchunkdims_;   // The chunk sizes of the extra (virtual) dimensions: {Y, X, n}
    Codec_t codec;             // Definition of codec used to compress the data.
};


#endif

