function read_nd_netcdf, file, range=range, uniqueId=uniqueId, timeStamp=timeStamp, dimInfo=dimInfo, arrayInfo=arrayInfo

;+
; NAME:
;   READ_ND_NETCDF
;
; PURPOSE:
;   This function reads a netCDF file written by the NDPluginFile plugin in the areaDetector module.
;   It is currently limited to reading the entire array, so may fail for very large files.
;   In the future it will be enhanced to support reading only a subset of the data.
;
; CATEGORY:
;   Detectors.
;
; CALLING SEQUENCE:
;   Data = READ_ND_NETCDF(File, Range=Range, UniqueId=UniqueId, TimeStamp=TimeStamp, DimInfo=DimInfo, ArrayInfo=ArrayInfo)
;
; INPUTS:
;   File:
;       The name of the input file.  If this argument is missing then the dialog_pickfile() function
;       will be called to produce a file browser to select a file interactively.
;
; KEYWORD INPUTS:
;  Range:
;       The range to read for each dimension.  If this keyword is missing then the entire file is read.
;       Specify as a 2D array.  -1 for any dimension means the
;       full value in the file. Only dimensions up to the last one to be limited need to be included
;       in the array, i.e. for a 2-D array range=[[0,10]] is equivalent to range=[[0,10],[-1,-1]]
;       Examples:
;         range=[[-1,-1], [0,100], [2,5]]
;       means read the full range of the data in the first (fastest) dimension, elements 0 to 100 for
;       the second dimension and elements 2 to 5 for the last (slowest) dimension.
;
; OUTPUTS:
;       This function returns the N-Dimensional array of data.  
;       The dimensions are [dim0, dim1, ..., NumArrays]
;
; KEYWORD OUTPUTS:
;   UniqueId: The uniqueID for each array.  These values can be used to correlate the data with
;             other EPICS scan data, and to determine which if any array data callbacks were dropped.
;
;   TimeStamp: The double precision time stamp in seconds for each array.
;
;   DimInfo: An array of structures of the following type containing the size, offset, binning and reverse fields
;            for each dimension:
;            dimInfo = {netCDFDimInfo, $
;                       size:    0L, $
;                       offset:  0L, $
;                       binning: 0L, $
;                       reverse:  0L}
;
;   ArrayInfo: A structure of the following type containing the color mode and Bayer color pattern:
;            arrayInfo = {netCDFArrayInfo, $
;                       colorMode:    "", $
;                       bayerPattern: ""}
;            Note that color support was only added to the netCDF files with R1-4 of areaDetector
;            in January 2009.  File before that were all "Mono".
;
; EXAMPLE:
;   IDL> data=read_nd_netcdf('testB_3.nc', uniqueId=uniqueId, timeStamp=timeStamp, dimInfo=dimInfo, arrayInfo=arrayInfo) 
;   IDL> help, /structure, data, uniqueId, timeStamp, dimInfo, arrayInfo                                                
;   DATA            BYTE      = Array[3, 1360, 1024, 10]
;   UNIQUEID        LONG      = Array[10]
;   TIMESTAMP       DOUBLE    = Array[10]
;   ** Structure NETCDFDIMINFO, 4 tags, length=16, data length=16:
;      SIZE            LONG                 3
;      OFFSET          LONG                 0
;      BINNING         LONG                 1
;      REVERSE         LONG                 0
;   ** Structure NETCDFARRAYINFO, 2 tags, length=24, data length=24:
;      COLORMODE       STRING    'RGB1'
;      BAYERPATTERN    STRING    'RGGB'
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, April 17, 2008
;   January 25, 2009 Mark Rivers Added arrayInfo support and RANGE keyword to read only a subset of the data
;                    Changed uniqueId, timeStamp and dimInfo to be keyword rather than positional
;-

    if (n_elements(file) eq 0) then file=dialog_pickfile(/must_exist)
    ncdf_control, 0, /noverbose
    file_id = ncdf_open(file, /nowrite)
    if (n_elements(file_id) eq 0) then begin
        ; This is not a netCDF file
        message, 'File is not netCDF file'
    endif
    ; This is a netCDF file
    ; Process the global attributes
    dimInfo = {netCDFDimInfo, $
               size:    0L, $
               offset:  0L, $
               binning: 0L, $
               reverse:  0L}
    ncdf_attget, file_id, /global, 'numArrayDims', ndims
    dimInfo = replicate(dimInfo, ndims)
    ncdf_attget, file_id, /global, "dimSize", size
    ncdf_attget, file_id, /global, "dimOffset", offset
    ncdf_attget, file_id, /global, "dimBinning", binning
    ncdf_attget, file_id, /global, "dimReverse", reverse
    for i=0, ndims-1 do begin
        dimInfo[i].size   = size[i]
        dimInfo[i].offset = offset[i]
        dimInfo[i].binning = binning[i]
        dimInfo[i].reverse = reverse[i]
    endfor
    
    ; Color mode and Bayer pattern attributes
    ; These don't exist in earlier versions, must check
    arrayInfo = {netCDFArrayInfo, $
                 colorMode:    "", $
                 bayerPattern: ""}
    arrayInfo.colorMode = 'Mono'
    arrayInfo.bayerPattern = 'RGGB'
    result = ncdf_attinq(file_id, /global, 'colorMode')
    if (result.datatype eq 'CHAR') then begin
        ncdf_attget, file_id, /global, 'colorMode', colorMode
        arrayInfo.colorMode = string(colorMode)
    endif
    result = ncdf_attinq(file_id, /global, 'bayerPattern')
    if (result.datatype eq 'CHAR') then begin
        ncdf_attget, file_id, /global, 'bayerPattern', bayerPattern
        arrayInfo.bayerPattern = string(bayerPattern)
    endif
    
    ; Get the data variable ids
    array_data_id   = ncdf_varid (file_id, 'array_data')
    uniqueId_id   = ncdf_varid (file_id, 'uniqueId')
    timeStamp_id   = ncdf_varid (file_id, 'timeStamp')

    if (array_data_id eq -1) then begin
        ncdf_close, file_id
        message, 'No array_data variable in netCDF file'
    endif

    ; Get information about the volume variable
    data_info = ncdf_varinq(file_id, array_data_id)

    ; If we are to read the entire array things are simpler
    if (n_elements(range) eq 0) then begin
        ncdf_varget, file_id, array_data_id, data
        ncdf_varget, file_id, uniqueId_id, uniqueId
        ncdf_varget, file_id, timeStamp_id, timeStamp
    endif else begin
        dims = size(range, /dimensions)
        if (dims[0] ne 2) then message, 'range must be be [2,x] array'
        if (n_elements(dims) eq 1) then range = reform(range, dims[0], 1)
        if (n_elements(dims) gt 2) then message, 'range must be be 1D or 2D array'
        ndims = data_info.ndims
        ; Initialize the range to read in each dimension to [-1, -1]
        min_range = lonarr(ndims) - 1
        max_range = lonarr(ndims) - 1
        dims = size(range, /dimensions)
        for dim=0, dims[1]-1 do begin
            min_range[dim] = range[0,dim]
            max_range[dim] = range[1,dim]
        endfor
        for dim=0, ndims-1 do begin
            ncdf_diminq, file_id, data_info.dim[dim], name, n
            min_range[dim] = (min_range[dim] > 0) < (n-1)
            if (max_range[dim] lt 0) then max_range[dim] = n-1
            max_range[dim] = (max_range[dim] > min_range[dim]) < (n-1)
        endfor
        ncdf_varget, file_id, array_data_id, data, $
            offset = min_range, $
            count = max_range - min_range + 1
        ncdf_varget, file_id, uniqueId_id, uniqueId, $
            offset = min_range[ndims-1], $
            count = max_range[ndims-1] - min_range[ndims-1] + 1
        ncdf_varget, file_id, timeStamp_id, timeStamp, $
            offset = min_range[ndims-1], $
            count = max_range[ndims-1] - min_range[ndims-1] + 1
    endelse

    ; Close the netCDF file
    ncdf_close, file_id

    return, data
end
