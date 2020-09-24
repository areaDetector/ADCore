#include <NDPluginAPI.h>
#include <epicsTypes.h>

/* These functions convert Mono12p and Mono12Packed formats to UInt16.
  The following description of these formats is from this document:
  http://softwareservices.flir.com/BFS-U3-51S5P/latest/Model/public/ImageFormatControl.html
   
  12-bit pixel formats have two different packing formats as defined by USB3 Vision and GigE Vision. 
  Note: the packing format is not related to the interface of the camera. Both may be available on USB3 or GigE devices.

  The USB3 Vision method is designated with a p. 
  It is a 12-bit format with its bit-stream following the bit packing method illustrated in Figure 3. 
  The first byte of the packed stream contains the eight least significant bits (lsb) of the first pixel. 
  The third byte contains the eight most significant bits (msb) of the second pixel. 
  The four lsb of the second byte contains four msb of the first pixel, 
  and the rest of the second byte is packed with the four lsb of the second pixel.

  This packing format is applied to: Mono12p, BayerGR12p, BayerRG12p, BayerGB12p and BayerBG12p.

  The GigE Vision method is designated with Packed. 
  It is a 12-bit format with its bit-stream following the bit packing method illustrated in Figure 4. 
  The first byte of the packed stream contains the eight msb of the first pixel. 
  The third byte contains the eight msb of the second pixel. 
  The four lsb of the second byte contains four lsb of the first pixel, 
  and the rest of the second byte is packed with the four lsb of the second pixel.

  This packing format is applied to: Mono12Packed, BayerGR12Packed, BayerRG12Packed, BayerGB12Packed and BayerBG12Packed.
*/
void NDPLUGIN_API decompressMono12p(int numPixels, epicsUInt8 *input, epicsUInt16 *output)
{
    /* Unpack a buffer that is packed with the USB3 Vision Mono12p compression
     * This routine assumes that the output array has already been allocated */
    int i;

    for (i=0; i<numPixels/2; i++) {
        *output++ = (*input << 4) | ((*(input+1) & 0x0f) << 12);
        *output++ = (*(input+1) & 0xf0) | (*(input+2) << 8);
        input += 3;
    }
}

void NDPLUGIN_API decompressMono12Packed(int numPixels, epicsUInt8 *input, epicsUInt16 *output)
{
    /* Unpack a buffer that is packed with the GigE Vision Mono12Packed compression
     * This routine assumes that the output array has already been allocated */
    int i;

    for (i=0; i<numPixels/2; i++) {
        *output++ = (*input << 8) | ((*(input+1) & 0x0f) << 4);
        *output++ = (*(input+1) & 0xf0) | (*(input+2) << 8);
        input += 3;
    }
}
