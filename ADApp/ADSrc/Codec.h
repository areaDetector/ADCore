#ifndef Codec_H
#define Codec_H

#include <string>

static std::string codecName[] = {
    "",
    "jpeg",
    "zlib",
    "blosc",
    "lz4",
    "lz4hdf5",
    "bslz4"
};

typedef enum {
  NDCODEC_NONE,
  NDCODEC_JPEG,
  NDCODEC_ZLIB,
  NDCODEC_BLOSC,
  NDCODEC_LZ4,
  NDCODEC_LZ4HDF5,
  NDCODEC_BSLZ4
} NDCodecCompressor_t;

typedef struct Codec_t {
  std::string name;       /**< Name of the codec used to compress the data. codecName[NDCODEC_NONE] if uncompressed. */
  int         level;      /**< Compression level. */
  int         shuffle;    /**< Shuffle type. */
  int         compressor; /**< Compressor type. For codecs that support more than one compressor. */

  Codec_t() {
    clear();
  }

  void clear() {
    name = codecName[NDCODEC_NONE];
    level = -1;
    shuffle = -1;
    compressor = -1;
  }

  bool empty() {
    return this->name == codecName[NDCODEC_NONE];
  }

  bool operator==(const Codec_t& other) {
    if (name == other.name &&
        level == other.level &&
        shuffle == other.shuffle &&
        compressor == other.compressor) {
      return true;
    } else {
      return false;
    }
  }
  bool operator!=(const Codec_t& other) {
    return ! (*this == other);
  }
} Codec_t;

#endif //Codec_H
