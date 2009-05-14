function epics_nd_std_arrays::newArray

    ; There is a bug in ezca, it takes a very long time for monitors on large arrays to be detected
    ; So instead we don't set a monitor on the data array, we set a monitor on the counter PV and
    ; do a caGet on the image data.
    prop = self->findProperty('ArrayCounter_RBV')
    new = caCheckMonitor(prop.pvName)
    ; Read the property to clear the monitor flag
    t = self->getProperty('ArrayCounter_RBV')
    return, new
end

function epics_nd_std_arrays::getArray

    ndims = self->getProperty('NDimensions_RBV')
    if (ndims lt 1) then message, 'No data available, ndims=0'
    dims = self->getProperty('Dimensions_RBV')
    dims = dims[0:ndims-1]
    nelements = 1L
    for i=0, ndims-1 do nelements = nelements * dims[i]
    if (nelements lt 1) then message, 'No data available, 1 or more dimensions=0'
    status = caGet(self.prefix + 'ArrayData', data, max=nelements)
    data = reform(data, dims, /overwrite)
    ; There is another complication.  EPICS CA does not handle the USHORT data type well,
    ; it promotes it to LONG.  So the datatype of the array may appear to be 16-bit signed
    ; but it could really be unsigned.  Use the DataType_RBV value to see if we need to
    ; convert
    dataType = self->getProperty('DataType_RBV')
    if (dataType eq 'UInt16') then data = uint(data)
    return, data
end


function epics_nd_std_arrays::init, prefix
;+
; NAME:
;       EPICS_ND_STD_ARRAYS::INIT
;
; PURPOSE:
;       This is the initialization code which is invoked when a new object of
;       type EPICS_ND_STD_ARRAYS is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_ND_STD_ARRAYS')
;
; INPUTS:
;       Prefix:  The prefix string for the EPICS PVs defined in ADBase.template
;
; OUTPUTS:
;       This function returns success (1) if it was able to connect to all of the
;       EPICS PVs for this object.  It returns failure (0) if is was unable to
;       connect to any EPICS PV.
;
; RESTRICTIONS:
;       This function only works on systems that have the EPICS IDL Channel Access
;       software installed.  This requires the shareable object ezcaIDL.dll or
;       ezcaIDL.so.  The EZCA_IDL_SHARE environment variable must point to this
;       shareable object file.  It must be a version of this library built with
;       EPICS 3.14 to transfer arrays larger than 16,000 bytes.
;       The EPICS_CA_MAX_ARRAY_BYTES environment variable must be set to a value
;       at least as large as the biggest array that will be used.
;
; EXAMPLE:
;       d = obj_new('EPICS_ND_STD_ARRAYS')
;       d->setProperty, 'Acquire', 1
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008
;-

    status = self->epics_ndplugin_base::init(prefix)
    if (status ne 1) then return, status
    self.prefix = prefix

    catch, err
    if (err ne 0) then begin
       print, !ERROR_STATE.MSG
       return, 0
    endif

    catch, /cancel

    return, 1
end

pro epics_nd_std_arrays__define
;+
; NAME:
;       EPICS_ND_STD_ARRAYS__DEFINE
;
; PURPOSE:
;       This is the definition code which is invoked when a new object of
;       type EPICS_ND_STD_ARRAYS is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.  It defines the data
;       structures used for the EPICS_ND_STD_ARRAYS class.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_ND_STD_ARRAYS')
;
; INPUTS:
;       None
;
; OUTPUTS:
;       None
;
; RESTRICTIONS:
;       This routine cannot be called directly.  It is called indirectly when
;       creating a new object of class EPICS_ND_STD_ARRAYS by the IDL OBJ_NEW()
;       function.
;
; EXAMPLE:
;       d = obj_new('EPICS_ND_STD_ARRAYS')
;       Time = d->getProperty('AcquireTime_RBV')
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008
;-
    epics_nd_std_arrays = {epics_nd_std_arrays, $
                           INHERITS epics_ndplugin_base}
 end
