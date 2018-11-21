#include <stdlib.h>
#include <stdio.h>
#include "jpeglib.h"

#include <epicsExport.h>


/*
 * The result of a decompressed JPEG will be either:
 *   - 8-bit mono
 *   - 8-bit RGB1
 */
epicsShareFunc void decompressJPEG(unsigned char *input, unsigned long compressedSize, unsigned char *output)
{
    struct jpeg_decompress_struct jpegInfo;
    struct jpeg_error_mgr jpegErr;

    jpeg_create_decompress(&jpegInfo);
    jpegInfo.err = jpeg_std_error(&jpegErr);

    jpeg_mem_src(&jpegInfo, input,  compressedSize);
    jpeg_read_header(&jpegInfo, TRUE);
    jpeg_start_decompress(&jpegInfo);

    while (jpegInfo.output_scanline < jpegInfo.output_height) {
        unsigned char *row_pointer[1] = { output };

        if (jpeg_read_scanlines(&jpegInfo, row_pointer, 1) != 1) {
            fprintf(stderr, "decompressJPEG: error decoding JPEG\n");
            break;
        }
        output += jpegInfo.output_width*jpegInfo.output_components;
    }

    jpeg_finish_decompress(&jpegInfo);
}
