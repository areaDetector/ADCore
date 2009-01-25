/* NDFileNetCDF.c
 * Writes NDArrays to netCDF files.
 *
 * Mark Rivers
 * April 17, 2008
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#include "NDArray.h"
#include "NDFileNetCDF.h"

/* Handle errors by printing an error message and exiting with a
 * non-zero status. */
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return(ERRCODE);}

int NDFileWriteNetCDF(const char *fileName, NDFileNetCDFState_t *pState, 
                      NDArray *pArray, int numArrays, int append, int close)
{
    /* When we create netCDF variables and dimensions, we get back an
     * ID for each one. */
    int dimIds[ND_ARRAY_MAX_DIMS+1];
    size_t start[ND_ARRAY_MAX_DIMS+1], count[ND_ARRAY_MAX_DIMS+1];
    int size[ND_ARRAY_MAX_DIMS], offset[ND_ARRAY_MAX_DIMS];
    int binning[ND_ARRAY_MAX_DIMS], reverse[ND_ARRAY_MAX_DIMS];
    char dimName[25];
    int retval;
    nc_type ncType=NC_NAT;
    static const char *colorModes[] = {"Mono", "Bayer", "RGB1", "RGB2", "RGB3", "YUV444", "YUV422", "YUV421"};
    static const char *bayerPatterns[] = {"RGGB", "GBRG", "GRBG", "BGGR"};
    int i, j;
    int dim0;

    if (!append) {
        /* Set the next record in the file to 0 */
        pState->nextRecord = 0;

        /* Create the file. The NC_CLOBBER parameter tells netCDF to
         * overwrite this file, if it already exists.*/
        if ((retval = nc_create(fileName, NC_CLOBBER, &pState->ncId)))
            ERR(retval);

        /* Create global attribute for the data type because netCDF does not
         * distinguish signed and unsigned.  Readers can use this to know how to treat
         * integer data. */
        if ((retval = nc_put_att_int(pState->ncId, NC_GLOBAL, "dataType", 
                                     NC_INT, 1, (const int*)&pArray->dataType)))
            ERR(retval);

        /* Create global attribute for number of dimensions and dimensions
         * in each NDArray.
         * This is redundant with information netCDF puts in, but the netCDF
         * info includes the number of arrays in the file.  This can make it
         * easier to write readers */
        if ((retval = nc_put_att_int(pState->ncId, NC_GLOBAL, "numArrayDims", 
                                     NC_INT, 1, &pArray->ndims)))
            ERR(retval);

        /* Define the dimensions. NetCDF will hand back an ID for each.
         * netCDF has the first dimension changing slowest, opposite of NDArrayBuff
         * convention. We make the first dimension the number of arrays we were passed */
        dim0 = numArrays;
        if (numArrays < 0) {
            numArrays = -numArrays;
            dim0 = NC_UNLIMITED;
        }
        if ((retval = nc_def_dim(pState->ncId, "numArrays", dim0, &dimIds[0])))
            ERR(retval);

        /* The next dimensions are the dimensions of the data in reversed order */
        for (i=0; i<pArray->ndims; i++) {
            j = pArray->ndims - i - 1;
            sprintf(dimName, "dim%d", i);
            if ((retval = nc_def_dim(pState->ncId, dimName, pArray->dims[j].size, &dimIds[i+1])))
                ERR(retval);
            size[i]    = pArray->dims[i].size;
            offset[i]  = pArray->dims[i].offset;
            binning[i] = pArray->dims[i].binning;
            reverse[i]  = pArray->dims[i].reverse;
        }

        /* Create global attribute for information about the dimensions */
        if ((retval = nc_put_att_int(pState->ncId, NC_GLOBAL, "dimSize", 
                                     NC_INT, pArray->ndims, size)))
            ERR(retval);
        if ((retval = nc_put_att_int(pState->ncId, NC_GLOBAL, "dimOffset", 
                                     NC_INT, pArray->ndims, offset)))
            ERR(retval);
        if ((retval = nc_put_att_int(pState->ncId, NC_GLOBAL, "dimBinning", 
                                     NC_INT, pArray->ndims, binning)))
            ERR(retval);
        if ((retval = nc_put_att_int(pState->ncId, NC_GLOBAL, "dimReverse", 
                                     NC_INT, pArray->ndims, reverse)))
            ERR(retval);
            
        if ((retval = nc_put_att_text(pState->ncId, NC_GLOBAL, "colorMode", 
                                     strlen(colorModes[pArray->colorMode]),
                                     colorModes[pArray->colorMode])))
            ERR(retval);

        if ((retval = nc_put_att_text(pState->ncId, NC_GLOBAL, "bayerPattern", 
                                     strlen(bayerPatterns[pArray->bayerPattern]),
                                     bayerPatterns[pArray->bayerPattern])))
            ERR(retval);
            
        /* Convert from NDArray data types to netCDF data types */
        switch (pArray->dataType) {
            case NDInt8:
            case NDUInt8:
                ncType = NC_BYTE;
                break;
            case NDInt16:
            case NDUInt16:
                ncType = NC_SHORT;
                break;
            case NDInt32:
            case NDUInt32:
                ncType = NC_INT;
                break;
            case NDFloat32:
                ncType = NC_FLOAT;
                break;
            case NDFloat64:
                ncType = NC_DOUBLE;
                break;
        }

        /* Define the unique data variable. */
        if ((retval = nc_def_var(pState->ncId, "uniqueId", NC_INT, 1, 
			         &dimIds[0], &pState->uniqueIdId)))
            ERR(retval);

        /* Define the timestamp data variable. */
        if ((retval = nc_def_var(pState->ncId, "timeStamp", NC_DOUBLE, 1, 
			         &dimIds[0], &pState->timeStampId)))
            ERR(retval);

        if ((retval = nc_def_var(pState->ncId, "array_data", ncType, pArray->ndims+1,
			         dimIds, &pState->arrayDataId)))
            ERR(retval);
            
        /* End define mode. This tells netCDF we are done defining
         * metadata. */
        if ((retval = nc_enddef(pState->ncId)))
            ERR(retval);
    }
    
    /* We are done defining the netCDF file and are now in "data" mode */
        
    count[0] = 1;
    start[0] = pState->nextRecord;
    for (i=0; i<pArray->ndims; i++) {
        j = pArray->ndims - i - 1;
        count[i+1] = pArray->dims[j].size;
        start[i+1] = 0;
    }
 
    for (i=0; i<numArrays; i++, pArray++, start[0]++) {
        /* Write the data to the file. */
        if ((retval = nc_put_vara_int(pState->ncId, pState->uniqueIdId, start, count, &pArray->uniqueId)))
                    ERR(retval);
        if ((retval = nc_put_vara_double(pState->ncId, pState->timeStampId, start, count, &pArray->timeStamp)))
                    ERR(retval);

        switch (pArray->dataType) {
            case NDInt8:
            case NDUInt8:
                if ((retval = nc_put_vara_schar(pState->ncId, pState->arrayDataId, start, count, (signed char*)pArray->pData)))
                    ERR(retval);
                break;
            case NDInt16:
            case NDUInt16:
                if ((retval = nc_put_vara_short(pState->ncId, pState->arrayDataId, start, count, (short *)pArray->pData)))
                    ERR(retval);
                break;
            case NDInt32:
            case NDUInt32:
                if ((retval = nc_put_vara_int(pState->ncId, pState->arrayDataId, start, count, (int *)pArray->pData)))
                    ERR(retval);
                break;
            case NDFloat32:
                if ((retval = nc_put_vara_float(pState->ncId, pState->arrayDataId, start, count, (float *)pArray->pData)))
                    ERR(retval);
                break;
            case NDFloat64:
                if ((retval = nc_put_vara_double(pState->ncId, pState->arrayDataId, start, count, (double *)pArray->pData)))
                    ERR(retval);
                break;
        }
    }
    pState->nextRecord++;

    if (close) {
        /* Close the file. This frees up any internal netCDF resources
         * associated with the file, and flushes any buffers. */
        if ((retval = nc_close(pState->ncId)))
            ERR(retval);
    }

    return 0;
}
