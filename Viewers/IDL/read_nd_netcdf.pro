function read_nd_netcdf, file, range=range, attributes=attributes, dimInfo=dimInfo

;+
; NAME:
;   READ_ND_NETCDF
;
; PURPOSE:
;   This function reads a netCDF file written by the NDPluginFile plugin in the areaDetector module.
;
; CATEGORY:
;   Detectors.
;
; CALLING SEQUENCE:
;   Data = READ_ND_NETCDF(File, Range=Range, Attributes=Attributes, DimInfo=DimInfo)
;
; INPUTS:
;   File:
;       The name of the input file.  If this argument is missing then the dialog_pickfile() function
;       will be called to produce a file browser to select a file interactively.
;
; KEYWORD INPUTS:
;  Range:
;       The range to read for each dimension.  If this keyword is missing then the entire file is read.
;       Specify as a 2D array.  [-1,-1] for any dimension means the
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
;   Attributes: An array of structures of the following type containing the name, description and pointer to value
;           for each additional variable (NDArray attribute) in the file:
;           attrInfo = {netCDFAttrInfo, $
;                       name:    "", $
;                       description: "", $
;                       pValue:  ptr_new()}
;
;
;   DimInfo: An array of structures of the following type containing the size, offset, binning and reverse fields
;            for each dimension:
;            dimInfo = {netCDFDimInfo, $
;                       size:    0L, $
;                       offset:  0L, $
;                       binning: 0L, $
;                       reverse:  0L}
;
; EXAMPLE:
;
; IDL> data = read_nd_netcdf('/home/epics/scratch/test_color_70.nc', attributes=attributes, dimInfo=dimInfo)
;
; IDL> help, data
; DATA            BYTE      = Array[3, 640, 480, 50]
;
; IDL> help, /structure, dimInfo
; ** Structure NETCDFDIMINFO, 4 tags, length=16, data length=16:
;    SIZE            LONG                 3
;    OFFSET          LONG                 0
;    BINNING         LONG                 1
;    REVERSE         LONG                 0
;
; IDL> help, /structure, attributes[3]
; ** Structure NETCDFATTRINFO, 3 tags, length=28, data length=28:
;   NAME            STRING    'I8Value'
;   DESCRIPTION     STRING    'Signed 8-bit time'
;   PVALUE          POINTER   <PtrHeapVar249>
;
; IDL> print, attributes[10].name
; F64Value
;
; IDL> help, *attributes[10].pValue
; <PtrHeapVar256> DOUBLE    = Array[50]
;
; IDL> print, (*attributes[10].pValue)[0:9], format='(f20.10)'
; 609361297.6659992933
; 609361297.6907505989
; 609361297.7110251188
; 609361297.7338150740
; 609361297.7543159723
; 609361297.7819019556
; 609361297.8048160076
; 609361297.8250850439
; 609361297.8466095924
; 609361297.8699282408
;
; Read just a subset of the array data.  Arrays 30-39, complete data in the other dimensions
; IDL> data = read_nd_netcdf('/home/epics/scratch/test_color_70.nc', attributes=attributes, dimInfo=dimInfo, $
;                           range=[[-1,-1],[-1,-1],[-1,-1],[30,39]])
; IDL> help, data
; DATA            BYTE      = Array[3, 640, 480, 10]
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, April 17, 2008
;   January 25, 2009 Mark Rivers Added arrayInfo support and RANGE keyword to read only a subset of the data
;                    Changed uniqueId, timeStamp and dimInfo to be keyword rather than positional
;   April 23, 2009  Mark Rivers.  Added support for new file format with NDArray attributes.  Removed uniqueId and timeStamp
;                   keywords, these are now handled under general attributes.  colorMode and bayerPattern no longer
;                   returned in arrayInfo structure, they are also general attributes of each array.
;-

    if (n_elements(file) eq 0) then file=dialog_pickfile(/must_exist)
    ncdf_control, 0, /noverbose
    file_id = ncdf_open(file, /nowrite)
    if (n_elements(file_id) eq 0) then begin
        ; This is not a netCDF file
        message, 'File is not netCDF file'
    endif
    ; This is a netCDF file
    ; Inquire about the file struucture
    fileStructure = ncdf_inquire(file_id)

    dimInfo = {netCDFDimInfo, $
               size:    0L, $
               offset:  0L, $
               binning: 0L, $
               reverse:  0L}
    attrInfo = {netCDFAttrInfo, $
               name:    "", $
               description: "", $
               pValue:  ptr_new()}

    ; Process the global attributes

    ; See what version of the file format this is.
    ; Prior to version 2.0 this attribute was not in the file.
    fileVersion = 1.0
    att_info = ncdf_attinq(file_id, /global, 'NDNetCDFFileVersion')
    if (att_info.datatype eq 'DOUBLE') then begin
        ncdf_attget, file_id, /global, 'NDNetCDFFileVersion', fileVersion
    endif

    ; Array data type (NDDataType_t enumeration)
    ncdf_attget, file_id, /global, 'dataType', dataType
    ; Array dimension information
    ncdf_attget, file_id, /global, 'numArrayDims', ndims
    dimInfo = replicate(dimInfo, ndims)
    ncdf_attget, file_id, /global, "dimSize", size
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

    ; Get the data variable id.  This must be present in every file.
    array_data_id   = ncdf_varid (file_id, 'array_data')

    if (array_data_id eq -1) then begin
        ncdf_close, file_id
        message, 'No array_data variable in netCDF file'
    endif

    ; Get information about the array variable
    data_info = ncdf_varinq(file_id, array_data_id)
    ; Get the number of arrays in the file
    ncdf_diminq, file_id, data_info.dim[0], name, numArrays
    ; Get the maximum attribute string size
    if (fileVersion ge 2.0) then begin
        dim_id = ncdf_dimid(file_id, 'attrStringSize')
        ncdf_diminq, file_id, dim_id, name, attrStringSize
    endif else begin
        attrStringSize = 256
    endelse

    ; There can be many other variables present in the file.
    ; These include uniqueId, timeStamp, and all of the NDArray attributes.
    ; Create an array of structures to hold data for these variables.
    attributes = replicate(attrInfo, fileStructure.nVars-1)
    attributeIds = intarr(fileStructure.nVars-1)
    numAttributes = 0
    for i=0, fileStructure.nVars-1 do begin
        var_info = ncdf_varinq(file_id, i)
        ; The main array data is not included in the attribute arrays
        if (var_info.name eq "array_data") then continue
        attributeIds[numAttributes] = i
        attributes[numAttributes].name = var_info.name
        attributes[numAttributes].description = var_info.name
        ; Now that we know the name there should be a global attribute
        ; with the description of this variable
        att_info = ncdf_attinq(file_id, /global, var_info.name+'_Description')
        if (att_info.datatype eq 'CHAR') then begin
            ncdf_attget, file_id, /global, var_info.name+'_Description', description
            attributes[numAttributes].description = string(description)
        endif
        numAttributes = numAttributes + 1
    endfor

    ; If we are to read the entire array things are simpler
    if (n_elements(range) eq 0) then begin
        ncdf_varget, file_id, array_data_id, data
        for i=0, numAttributes-1 do begin
            ncdf_varget, file_id, attributeIds[i], temp
            ; If the data type is CHAR then convert to string
            var_info = ncdf_varinq(file_id, attributeIds[i])
            if (var_info.datatype eq 'CHAR') then temp = string(temp)
            attributes[i].pValue = ptr_new(temp, /no_copy)
        endfor
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
        for i=0, numAttributes-1 do begin
            var_info = ncdf_varinq(file_id, attributeIds[i])
            ; Most variables will be 1-D.  But strings will be 2-D
            if (var_info.nDims eq 1) then begin
                offset = min_range[ndims-1]
                count = max_range[ndims-1] - min_range[ndims-1] + 1
            endif else begin
                offset = [0, min_range[ndims-1]]
                count = [attrStringSize, max_range[ndims-1] - min_range[ndims-1] + 1]
            endelse
            ncdf_varget, file_id, attributeIds[i], temp, $
                offset = offset, $
                count = count
            ; If the data type is CHAR then convert to string
            if (var_info.datatype eq 'CHAR') then temp = string(temp)
            attributes[i].pValue = ptr_new(temp, /no_copy)
        endfor
    endelse
    
    ; If the datatype is unsigned convert to signed
    if (dataType eq 3) then data = uint(data)
    if (dataType eq 5) then data = ulong(data)

    ; Close the netCDF file
    ncdf_close, file_id

    return, data
end
