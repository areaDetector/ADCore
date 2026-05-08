#ifndef NDCodec_H
#define NDCodec_H

#include <string>
#include "ADCoreAPI.h"

typedef enum {
  NDCODEC_NONE,
  NDCODEC_JPEG,
  NDCODEC_ZLIB,
  NDCODEC_BLOSC,
  NDCODEC_LZ4,
  NDCODEC_LZ4HDF5,
  NDCODEC_BSLZ4
} NDCodecCompressor_t;

#define NDCODEC_NUM_CODECS 7

extern ADCORE_API const std::string NDCodecName[NDCODEC_NUM_CODECS];

typedef enum {
    NDCODEC_BLOSC_BLOSCLZ,
    NDCODEC_BLOSC_LZ4,
    NDCODEC_BLOSC_LZ4HC,
    NDCODEC_BLOSC_SNAPPY,
    NDCODEC_BLOSC_ZLIB,
    NDCODEC_BLOSC_ZSTD
} NDCodecBloscComp_t;

#define NDCODEC_BLOSC_NUM_COMPRESSORS 6

extern ADCORE_API const std::string NDCodecBloscCompName[NDCODEC_BLOSC_NUM_COMPRESSORS];

typedef struct NDCodec_t {
  std::string name;       /**< Name of the codec used to compress the data. codecName[NDCODEC_NONE] if uncompressed. */
  int         level;      /**< Compression level. */
  int         shuffle;    /**< Shuffle type. */
  int         compressor; /**< Compressor type. For codecs that support more than one compressor. */

  NDCodec_t() {
    clear();
  }

  void clear() {
    name = NDCodecName[NDCODEC_NONE];
    level = -1;
    shuffle = -1;
    compressor = -1;
  }

  bool empty() {
    return this->name == NDCodecName[NDCODEC_NONE];
  }

  bool operator==(const NDCodec_t& other) {
    if (name == other.name &&
        level == other.level &&
        shuffle == other.shuffle &&
        compressor == other.compressor) {
      return true;
    } else {
      return false;
    }
  }
  bool operator!=(const NDCodec_t& other) {
    return ! (*this == other);
  }
} NDCodec_t;

#endif //NDCodec_H
