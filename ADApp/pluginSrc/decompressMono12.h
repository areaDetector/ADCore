/* decompressMono12.h
 * Mark Rivers
 */

#ifndef DECOMPRESS_MONO12_H
#define DECOMPRESS_MONO12_H

#ifdef __cplusplus
extern "C" {
#endif

#include <NDPluginAPI.h>

void NDPLUGIN_API decompressMono12p(int numPixels, epicsUInt8 *input, epicsUInt16 *output);
void NDPLUGIN_API decompressMono12Packed(int numPixels, epicsUInt8 *input, epicsUInt16 *output);

#ifdef __cplusplus
}
#endif

#endif  /* DECOMPRESS_MONO12_H */
