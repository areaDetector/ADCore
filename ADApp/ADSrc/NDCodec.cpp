#include "NDCodec.h"

const std::string NDCodecName[NDCODEC_NUM_CODECS] = {
    "",
    "jpeg",
    "zlib",
    "blosc",
    "lz4",
    "lz4hdf5",
    "bslz4"
};

const std::string NDCodecBloscCompName[NDCODEC_BLOSC_NUM_COMPRESSORS] = {
    "blosclz",
    "lz4",
    "lz4hc",
    "snappy",
    "zlib",
    "zstd",
};
