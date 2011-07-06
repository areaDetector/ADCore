/* test_big_classic.c
 * Test program to write large (>4GB files in netCDF classic format
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#define MAX_DIMS 3
#define XSIZE 1024
#define YSIZE 1024
#define NUM_RECORDS 4100

/* Handle errors by printing an error message and exiting with a
 * non-zero status. */
#define ERR(e) {printf("error=%s\n", \
                nc_strerror(e)); \
                return(-1);}

int main(int argc, char **argv)
{
    /* When we create netCDF variables and dimensions, we get back an
     * ID for each one. */
    int dimIds[MAX_DIMS];
    size_t start[MAX_DIMS] = {0, 0, 0};
    size_t count[MAX_DIMS] = {1, YSIZE, XSIZE};
    int ncId;
    int dataId;
    int retval;
    int i;
    char *fileName = argv[1];
    char *pData = calloc(XSIZE, YSIZE);

    if (argc != 2) {
        printf("Usage: test_big_classic filename\n");
        return(-1);
    }
    
    /* Create the file. The NC_CLOBBER parameter tells netCDF to
     * overwrite this file, if it already exists.  No other flags, so it is classic format. */
    printf("Creating file %s\n", fileName);
    if ((retval = nc_create(fileName, NC_CLOBBER, &ncId)))
        ERR(retval);

    /* Define the dimensions */
    if ((retval = nc_def_dim(ncId, "numArrays", NC_UNLIMITED, &dimIds[0])))
        ERR(retval);

    if ((retval = nc_def_dim(ncId, "YSize", YSIZE, &dimIds[1])))
        ERR(retval);

    if ((retval = nc_def_dim(ncId, "XSize", XSIZE, &dimIds[2])))
        ERR(retval);

    /* Define the array data variable. */
    if ((retval = nc_def_var(ncId, "array_data", NC_BYTE, 3,
                 dimIds, &dataId)))
        ERR(retval);

    /* End define mode. This tells netCDF we are done defining
     * metadata. */
    if ((retval = nc_enddef(ncId)))
        ERR(retval);
        
    /* Write data to file */
    for (i=0; i<NUM_RECORDS; i++) {
      printf("writing record %d/%d\n", i+1, NUM_RECORDS);
        start[0] = i;
        if ((retval = nc_put_vara_schar(ncId, dataId, start, count, (signed char*)pData)))
            ERR(retval);
    }
    printf("Closing file %s\n", fileName);
   if ((retval = nc_close(ncId)))
        ERR(retval);

   return(0);
}

