; test_big_classic.pro
; Test program to write large (>4GB files in netCDF classic format

pro test_big_classic, fileName

    MAX_DIMS = 3
    XSIZE = 1024
    YSIZE = 1024
    NUM_RECORDS = 4100

    dimIds = intarr(MAX_DIMS);
    start = [0L,  0L, 0L];
    count = [YSIZE, XSIZE, 1L];
    pData = bytarr(XSIZE, YSIZE);

    ; Create the file. The NC_CLOBBER parameter tells netCDF to
    ; overwrite this file, if it already exists.  No other flags, so it is classic format.
    print, "Creating file ", fileName
    ncId = ncdf_create(fileName, /clobber)
    ncdf_control, ncId, fill=0

    ; Define the dimensions
    dimIds[2] = ncdf_dimdef(ncId, 'numArrays', /unlimited)
    dimIds[1] = ncdf_dimdef(ncId, 'YSize', YSIZE)
    dimIds[0] = ncdf_dimdef(ncId, 'XSize', XSIZE)

    ; Define the array data variable
    dataId = ncdf_vardef(ncId, 'array_data', dimIds, /BYTE)

    ; End define mode. This tells netCDF we are done defining
    ; metadata.
    ncdf_control, ncId, /endef

    ; Write data to file
    for i=0, NUM_RECORDS-1 do begin
        print, "writing record", i+1, "/", NUM_RECORDS
        start[2] = i;
        ncdf_varput, ncId, dataId, pData, $
                     offset=start, count=count
    endfor
    print, "Closing file", fileName
    ncdf_close, ncId
end

